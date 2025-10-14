#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <crow.h>

#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#include <grp.h>
#include <string>
#include <vector>
#include <optional>

#define SYS_MOUSE_EVENT 548
#define SYS_SEND_KEY_EVENT 552
#define get_sys_usage 554

/* -------------------------------------------------------------------------------------------- */

static const char* PAM_SERVICE_NAME = "login";

static int pam_conv_cb(int num_msg, const struct pam_message **msg,
                       struct pam_response **resp, void *appdata_ptr) {
    if (num_msg <= 0) return PAM_CONV_ERR;
    auto *responses = (pam_response*)calloc(num_msg, sizeof(pam_response));
    if (!responses) return PAM_CONV_ERR;

    const char *password = (const char *)appdata_ptr;
    for (int i = 0; i < num_msg; i++) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
                responses[i].resp = strdup(password ? password : "");
                responses[i].resp_retcode = 0;
                break;
            case PAM_PROMPT_ECHO_ON:
            case PAM_ERROR_MSG:
            case PAM_TEXT_INFO:
                responses[i].resp = nullptr;
                responses[i].resp_retcode = 0;
                break;
            default:
                free(responses);
                return PAM_CONV_ERR;
        }
    }
    *resp = responses;
    return PAM_SUCCESS;
}

static bool pam_authenticate_user(const std::string& username,
                                  const std::string& password,
                                  std::string* pam_error_out = nullptr) {
    pam_handle_t* pamh = nullptr;
    struct pam_conv conv { pam_conv_cb, (void*)password.c_str() };

    int r = pam_start(PAM_SERVICE_NAME, username.c_str(), &conv, &pamh);
    if (r != PAM_SUCCESS) {
        if (pam_error_out) *pam_error_out = pam_strerror(pamh, r);
        if (pamh) pam_end(pamh, r);
        return false;
    }

    r = pam_authenticate(pamh, 0);
    if (r == PAM_SUCCESS) r = pam_acct_mgmt(pamh, 0);

    bool ok = (r == PAM_SUCCESS);
    if (!ok && pam_error_out) *pam_error_out = pam_strerror(pamh, r);
    pam_end(pamh, r);
    return ok;
}

static bool user_in_group(const std::string& username, const std::string& groupname) {
    struct passwd* pw = getpwnam(username.c_str());
    if (!pw) return false;

    struct group* gr = getgrnam(groupname.c_str());
    if (!gr) return false;
    gid_t target_gid = gr->gr_gid;

    int ngroups = 0;
    getgrouplist(username.c_str(), pw->pw_gid, nullptr, &ngroups);
    if (ngroups <= 0) return false;

    std::vector<gid_t> groups(ngroups);
    if (getgrouplist(username.c_str(), pw->pw_gid, groups.data(), &ngroups) == -1) {
        groups.resize(ngroups);
        if (getgrouplist(username.c_str(), pw->pw_gid, groups.data(), &ngroups) == -1)
            return false;
    }

    for (int i = 0; i < ngroups; ++i)
        if (groups[i] == target_gid) return true;
    return false;
}

enum class Access { None, View, Control };

static Access resolve_access(const std::string& username) {
    if (user_in_group(username, "remote_control")) return Access::Control;
    if (user_in_group(username, "remote_view"))   return Access::View;
    return Access::None;
}

/* -------------------------------------------------------------------------------------------- */

static long send_keycode(int keycode) {
    return syscall(SYS_SEND_KEY_EVENT, keycode);
}

// --- Middleware CORS ---
struct CORS {
    struct context {}; // Crow exige un 'context' aunque esté vacío

    void before_handle(crow::request& req, crow::response& res, context&) {
        if (req.method == crow::HTTPMethod::OPTIONS) {
            // Responder preflight inmediatamente
            res.add_header("Access-Control-Allow-Origin", "*");
            res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
            res.add_header("Access-Control-Allow-Methods", "GET,POST,PUT,PATCH,DELETE,OPTIONS");
            res.code = 204; // No Content
            res.end();
        }
    }

    void after_handle(crow::request&, crow::response& res, context&) {
        // Añadir siempre CORS a las respuestas normales
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.add_header("Access-Control-Allow-Methods", "GET,POST,PUT,PATCH,DELETE,OPTIONS");
    }
};

int main() {
    // Activa el middleware CORS
    crow::App<CORS> app;

    CROW_ROUTE(app, "/")([]{
        return "Hello world from C++;";
    });

    CROW_ROUTE(app, "/keyboard").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto json = crow::json::load(req.body);
        if (!json) {
            return crow::response(400, "JSON inválido");
        }
        if (!json.has("key")) {
            return crow::response(422, "Campo 'key' requerido");
        }
        if (json["key"].t() != crow::json::type::Number) {
            return crow::response(422, "'key' debe ser entero");
        }

        int keycode = json["key"].i();
        long rc = send_keycode(keycode);
        if (rc != 0) {
            return crow::response(500, std::string("syscall send_key_event fallo: ") + strerror(errno));
        }

        crow::json::wvalue body;
        body["received"] = keycode;
        body["status"] = "sent";
        return crow::response(201, body);
    });

    CROW_ROUTE(app, "/metrics").methods(crow::HTTPMethod::GET)
    ([](){
        int cpu_x100 = 0, ram_x100 = 0;
        long ret = syscall(get_sys_usage, &cpu_x100, &ram_x100);
        if (ret < 0) {
            return crow::response(500, std::string("syscall get_sys_usage fallo: ") + strerror(errno));
        }

        double cpu = cpu_x100 / 100.0;
        double ram = ram_x100 / 100.0;
        
        crow::json::wvalue body;
        body["cpu"] = cpu;        
        body["ram"] = ram;    
        body["status"] = "ok";

        return crow::response(200, body);
    });

    CROW_ROUTE(app, "/mouse").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto json = crow::json::load(req.body);
        if (!json) {
            return crow::response(400, "JSON inválido");
        }

        if (!(json.has("x") && json.has("y"))) {
            return crow::response(422, "Campos 'x' y 'y' son requeridos");
        }
        if (json["x"].t() != crow::json::type::Number ||
            json["y"].t() != crow::json::type::Number) {
            return crow::response(422, "'x' y 'y' deben ser enteros");
        }

        int x = json["x"].i();
        int y = json["y"].i();
        int click = 0;
        if (json.has("click")) {
            if (json["click"].t() != crow::json::type::Number) {
                return crow::response(422, "'click' debe ser entero (0,1,2)");
            }
            click = json["click"].i();
        }

        long rc = syscall(SYS_MOUSE_EVENT, x, y, click);
        if (rc != 0) {
            return crow::response(500, std::string("syscall SYS_MOUSE_EVENT fallo: ") + strerror(errno));
        }

        crow::json::wvalue body;
        body["x"] = x;
        body["y"] = y;
        body["click"] = click;
        body["status"] = "ok";
        return crow::response(201, body);
    });

    CROW_ROUTE(app, "/auth").methods(crow::HTTPMethod::POST)
    ([](const crow::request& req){
        auto json = crow::json::load(req.body);
        if (!json || !json.has("username") || !json.has("password")) {
            return crow::response(400, "JSON con 'username' y 'password' requerido");
        }

        std::string username = json["username"].s();
        std::string password = json["password"].s();
        std::string required = "view";
        if (json.has("required") && json["required"].t() == crow::json::type::String) {
            required = json["required"].s();
        }

        // 1) Autenticación PAM
        std::string pam_err;
        if (!pam_authenticate_user(username, password, &pam_err)) {
            crow::json::wvalue body;
            body["ok"] = false;
            body["error"] = "PAM: " + pam_err;
            return crow::response(401, body);
        }

        // 2) Resolución de rol por grupo
        Access role = resolve_access(username);
        std::string role_str = (role == Access::Control ? "remote_control"
                            : role == Access::View    ? "remote_view"
                                                        : "none");

        // 3) Autorización según 'required'
        bool need_control = (required == "control");
        bool can_access = false;
        if (need_control) {
            can_access = (role == Access::Control);
        } else {
            can_access = (role == Access::Control || role == Access::View);
        }

        crow::json::wvalue body;
        body["ok"] = true;
        body["username"] = username;
        body["role"] = role_str;
        body["required"] = required;
        body["can_access"] = can_access;
        if (!can_access) body["error"] = (need_control ?
            "Requiere pertenecer al grupo 'remote_control'" :
            "Requiere pertenecer a 'remote_view' o 'remote_control'");

        return crow::response(can_access ? 200 : 403, body);
    });


    app.port(18080).multithreaded().run();
}

/* 

Compilar: g++ main.cpp -lpthread -lpam -lpam_misc
Ejecutar: sudo ./a.out

*/