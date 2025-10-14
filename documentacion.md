<p style="font-size: 18px">
Universidad de San Carlos de Guatemala
<br>
Facultad de Ingeniería
<br>
Escuela de Ciencias y Sistemas
<br>
Sistemas Operativos 2
<br>
Aux. 
</p>

<br><br><br><br>

<h1 align="center" style="font-size: 40px; font-weight: bold;">Proyecto</h1>
<br><br>
<h1 align="center" style="font-size: 40px; font-weight: bold;">Manual Tecnico</h1>

<br><br><br>

<div align="center">

|  Carnet   | Nombre |
| :-------: | :----: |
| 201XXXXXX |        |

</div>

<br><br>

<h4 align="center" style="font-size: 18px; font-weight: bold;">Guatemala 16 de octubre 2025</h4>

---

<br><br><br><br>

---

# 📘 Documentación Técnica del Proyecto

## Paquetes para compilacion del Kernel

Install my-project with npm

```bash
 sudo apt install build-essential libncurses-dev bison flex libssl-dev libelf-dev fakeroot dwarves
```

## Descargar Kernel

Para descargar el Kernel a utilizar debemos visitar la pagina "https://www.kernel.org/", eligiremos "longterm" y luego copiamos el link para descargar el ejecutable.

```bash
  wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.12.41.tar.xz
```

## Descomprimir el ejecutable

```bash
 tar -xf linux-6.12.41.tar.xz
```

# <a name="encriptacion"></a>Llamadas al sistema personalizadas

# <a name="instalacion"></a>Implementación de llamadas

## Declara la llamada en el archivo syscalls.

```bash
cd linux-6.12.41/arch/x86/entry/syscalls
nano syscall_64.tbl

// Ingresamos lo siguiente

#   arquitectura  nombreLlamada    nombreFuncion
548 common mouse_event sys_mouse_event

549 common get_screen sys_get_screen
550 common capture_screen sys_capture_screen
551 common screen_capture sys_screen_capture

552 common send_key_event sys_send_key_event
553 common send_char_event sys_send_char_event
554 common get_sys_usage sys_get_sys_usage

/*

Donde:

#: Numero de la llamada en el sistema.
arquitectura: Especifica arquitectura valida para el sistema.
nombreLlamada: Es el identificador simbólico que un programa podría usar para referirse a estas funciones.
nombreFuncion: Este es el nombre de la función en el código del kernel que se ejecutará cuando se invoque la llamada al sistema.

*/
```

#### NOTA: El nombre de la llamada debe coincidir con el parametro de los archivos ".c", de lo contrario mostrara un error de función no reconocida o declarada.

## Modificación del Makefile e insertamos los archivos .c con el codigo.

```bash
cd linux-6.6.44/kernel/
nano Makefile

// Ingresamos lo siguiente

obj-y += syscalls_mouse.o syscall_key.o syscalls_metrics.o
```

# <a name="compilacion"></a>Compilación del Kernel

## Inicia proceso de compilación.

```bash
cd linux-6.12.41/
cp -v /boot/config-$(uname -r) .config
make localmodconfig
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
scripts/config --set-str CONFIG_SYSTEM_TRUSTED_KEYS ""
scripts/config --set-str CONFIG_SYSTEM_REVOCATION_KEYS ""

/*

-j4: Representa el numero de nucleos que se utilizaran para realizar la compilación, para este caso se utilizaron 4.

*/

fakeroot make -j4
```

## Verificación del proceso de compilación

```bash
// Si muestra un "0" quiere decir que no hubo errores y la compilación fue exitosa.

echo $?
```

## Instalación de modulos

```bash
sudo make modules_install
```

## Instalación del Kernel

```bash
sudo make install
```

## Reinicio del sistema

```bash
sudo reboot
```

# Implementacion de API

A continuación se describen los pasos realizados para modificar el kernel, implementar la seguridad mediante PAM (Pluggable Authentication Modules) y desarrollar la API REST con **Crow**, un microframework para C++.

### Paso 1: Configuración del entorno

1. Instalación de las dependencias necesarias:
   ```bash
   sudo apt install libpam0g-dev libcrow-all-dev
   ```
2. Creación del archivo principal `main.cpp`.
3. Configuración del puerto de ejecución en **18080**.

### Paso 2: Implementación de seguridad con PAM

- Se definió el servicio `PAM_SERVICE_NAME` como `"login"`.
- Se implementó una función de callback `pam_conv_cb` que gestiona las respuestas del usuario (contraseña, mensajes informativos, etc.).
- La función `pam_authenticate_user()` realiza la autenticación del usuario mediante:
  - `pam_start()` para iniciar la sesión PAM.
  - `pam_authenticate()` para validar credenciales.
  - `pam_acct_mgmt()` para verificar la cuenta.

### Paso 3: Control de acceso por grupos

- Se creó `user_in_group()` para verificar si el usuario pertenece a los grupos:
  - `remote_control` → acceso completo.
  - `remote_view` → acceso de solo lectura.
- La función `resolve_access()` determina el tipo de acceso (`None`, `View`, `Control`).

### Paso 4: Desarrollo de la API REST

Se construyó una API REST con rutas específicas usando **Crow**:

| Ruta        | Método | Descripción                                                                 |
| ----------- | ------ | --------------------------------------------------------------------------- |
| `/`         | GET    | Respuesta básica de prueba.                                                 |
| `/auth`     | POST   | Autenticación y autorización de usuarios mediante PAM y grupos del sistema. |
| `/keyboard` | POST   | Envía eventos de teclado mediante syscall personalizada.                    |
| `/mouse`    | POST   | Envía eventos del ratón (posición y clic).                                  |
| `/metrics`  | GET    | Devuelve el uso de CPU y RAM del sistema.                                   |

### Paso 5: Middleware de CORS

- Se añadió una estructura `CORS` que intercepta todas las solicitudes y agrega cabeceras:
  ```cpp
  res.add_header("Access-Control-Allow-Origin", "*");
  res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
  res.add_header("Access-Control-Allow-Methods", "GET,POST,PUT,PATCH,DELETE,OPTIONS");
  ```

### Paso 6: Ejecución

- Compilación:
  ```bash
  g++ main.cpp -lpthread -lpam -lpam_misc
  ```
- Ejecución con privilegios:
  ```bash
  sudo ./a.out
  ```

---

## 2.4 Documentación de fragmentos de código

### Ejemplo de autenticación PAM

```cpp
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
```

### Ejemplo de ruta REST para control del mouse

```cpp
CROW_ROUTE(app, "/mouse").methods(crow::HTTPMethod::POST)
([](const crow::request& req){
    auto json = crow::json::load(req.body);
    if (!(json.has("x") && json.has("y"))) {
        return crow::response(422, "Campos 'x' y 'y' son requeridos");
    }
    int x = json["x"].i();
    int y = json["y"].i();
    int click = json.has("click") ? json["click"].i() : 0;

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
```

### Ejemplo de middleware CORS

```cpp
struct CORS {
    struct context {};
    void before_handle(crow::request& req, crow::response& res, context&) {
        if (req.method == crow::HTTPMethod::OPTIONS) {
            res.add_header("Access-Control-Allow-Origin", "*");
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
    }
};
```

---

## Endpoints de la API

A continuación se presentan los principales endpoints del servicio junto con ejemplos de uso y estructura de los datos intercambiados.

### **Endpoint: /keyboard**

**URL:** `http://0.0.0.0:18080/keyboard`  
**Método:** `POST`  
**Descripción:** Envía un evento de teclado al sistema.

**Ejemplo de solicitud:**

```json
{
  "key": 30
}
```

**Respuesta esperada (201 - Created):**

```json
{
  "received": 30,
  "status": "sent"
}
```

---

### **Endpoint: /metrics**

**URL:** `http://0.0.0.0:18080/metrics`  
**Método:** `GET`  
**Descripción:** Devuelve el estado actual de uso del sistema (CPU y RAM).

**Ejemplo de respuesta:**

```json
{
  "status": "ok",
  "ram": 73.15,
  "cpu": 24.12
}
```

---

### Uso de Angular 19

#### ¿Por qué Angular 19?

Angular 19 fue elegido como el framework principal para el desarrollo del frontend debido a las siguientes razones:

1. **Modularidad y Escalabilidad:**
   Angular proporciona una arquitectura basada en módulos que facilita la organización del código y permite escalar la aplicación de manera eficiente.

2. **Herramientas Modernas:**
   Angular 19 incluye características avanzadas como el soporte mejorado para Signals, optimizaciones en el renderizado y mejoras en el rendimiento.

3. **Ecosistema Rico:**
   Angular cuenta con un ecosistema robusto que incluye herramientas como Angular CLI para la generación de componentes, servicios y módulos, así como bibliotecas oficiales para manejo de formularios y comunicación HTTP.

#### ¿Por qué se eligió Angular sobre otros frameworks?

- **Comparado con React:** Aunque React es más flexible, Angular ofrece una solución completa con herramientas integradas, eliminando la necesidad de configurar bibliotecas externas para tareas comunes.
- **Comparado con Vue:** Vue es más ligero, pero Angular es más adecuado para aplicaciones empresariales grandes debido a su estructura estricta y herramientas avanzadas.
- **Experiencia del Equipo:** El equipo de desarrollo ya tenía experiencia previa con Angular, lo que permitió acelerar el proceso de desarrollo y reducir la curva de aprendizaje.

En resumen, Angular 19 fue seleccionado por su capacidad para manejar aplicaciones complejas, su ecosistema integrado que facilita el desarrollo de aplicaciones modernas y escalables.

## 2.6 Documentación de componentes del proyecto web

### Componente Metrics

#### Pasos seguidos

1. Creación del componente `metrics` utilizando Angular CLI:
   ```bash
   ng generate component metrics
   ```
2. Implementación de la lógica para obtener métricas del sistema desde la API REST.
3. Configuración de la vista para mostrar el uso de CPU y RAM en tiempo real.

#### Fragmento de código

- **Lógica para obtener métricas:**

  ```typescript
  import { HttpClient } from "@angular/common/http";
  import { Component, OnInit } from "@angular/core";

  @Component({
    selector: "app-metrics",
    templateUrl: "./metrics.component.html",
    styleUrls: ["./metrics.component.scss"],
  })
  export class MetricsComponent implements OnInit {
    metrics: any;

    constructor(private http: HttpClient) {}

    ngOnInit(): void {
      this.http.get("/api/metrics").subscribe((data) => {
        this.metrics = data;
      });
    }
  }
  ```

### Componente Auth

#### Pasos seguidos

1. Creación del servicio de autenticación `auth-service`:
   ```bash
   ng generate service auth/auth-service
   ```
2. Implementación de métodos para iniciar sesión y almacenar tokens de autenticación.
3. Integración con el backend para validar credenciales mediante PAM.

#### Fragmento de código

- **Servicio de autenticación:**

  ```typescript
  import { Injectable } from "@angular/core";
  import { HttpClient } from "@angular/common/http";

  @Injectable({
    providedIn: "root",
  })
  export class AuthService {
    constructor(private http: HttpClient) {}

    login(username: string, password: string) {
      return this.http.post("/api/auth", { username, password });
    }
  }
  ```

### Componente Remote

#### Pasos seguidos

1. Creación del componente `remote` para controlar dispositivos de forma remota.
2. Implementación de rutas específicas para enviar eventos de teclado y ratón.
3. Configuración de la vista para interactuar con los controles remotos.

#### Fragmento de código

- **Lógica para enviar eventos de teclado:**

  ```typescript
  import { HttpClient } from "@angular/common/http";
  import { Component } from "@angular/core";

  @Component({
    selector: "app-remote",
    templateUrl: "./remote.component.html",
    styleUrls: ["./remote.component.scss"],
  })
  export class RemoteComponent {
    constructor(private http: HttpClient) {}

    sendKeyEvent(key: string): void {
      this.http.post("/api/keyboard", { key }).subscribe((response) => {
        console.log("Evento de teclado enviado:", response);
      });
    }
  }
  ```

### Componente Remote (Eventos fromEvent)

#### Explicación de los eventos

1. **Movimiento del Mouse**:

   - **Evento:** `pointermove`
   - **Descripción:** Captura el movimiento del mouse dentro del área designada y envía las coordenadas relativas al backend.
   - **Fragmento de código:**
     ```typescript
     fromEvent<PointerEvent>(area, "pointermove")
       .pipe(
         sampleTime(20),
         map((ev) => ({
           dx: Math.trunc(ev.movementX),
           dy: Math.trunc(ev.movementY),
         })),
         filter(({ dx, dy }) => this.focused && (dx !== 0 || dy !== 0)),
         concatMap(({ dx, dy }) => this.remote.move(dx, dy)),
         takeUntil(this.destroy$)
       )
       .subscribe();
     ```

2. **Clic Izquierdo/Derecho**:

   - **Evento:** `pointerdown`
   - **Descripción:** Detecta clics izquierdo o derecho dentro del área y envía el evento correspondiente al backend.
   - **Fragmento de código:**
     ```typescript
     fromEvent<PointerEvent>(area, "pointerdown")
       .pipe(
         concatMap((ev) => {
           if (!this.focused) area.focus();
           ev.preventDefault();
           const btn: 1 | 2 = ev.button === 0 ? 1 : 2;
           return this.remote.click(0, 0, btn);
         }),
         takeUntil(this.destroy$)
       )
       .subscribe();
     ```

3. **Teclado**:
   - **Evento:** `keydown`
   - **Descripción:** Captura eventos de teclado cuando el área está enfocada y envía el código de tecla al backend.
   - **Fragmento de código:**
     ```typescript
     fromEvent<KeyboardEvent>(area, "keydown")
       .pipe(
         filter(() => this.focused),
         map((ev) => {
           const code = ev.code;
           const keycode = LINUX_KEYCODE[code];
           if (keycode) ev.preventDefault();
           return keycode ?? 0;
         }),
         filter((kc) => kc > 0),
         concatMap((kc) => this.remote.sendKey(kc)),
         takeUntil(this.destroy$)
       )
       .subscribe();
     ```

---

## Documentación de Syscalls

#### 1. `syscall_key.c`

- **Propósito:**
  Este archivo implementa un teclado virtual en el kernel de Linux, permitiendo enviar eventos de teclado mediante una syscall personalizada.
- **Lógica relevante:**
  - Se asegura que el teclado virtual esté inicializado con `vkeyboard_ensure_created`.
  - Convierte caracteres ASCII a keycodes de Linux con la función `ascii_to_keycode`.
- **Fragmento de código:**

  ```c
  static int ascii_to_keycode(int ch, int *keycode, bool *need_shift)
  {
      *need_shift = false;

      if (ch >= 'a' && ch <= 'z') {
          *keycode = KEY_A + (ch - 'a');
          return 0;
      }
      if (ch >= 'A' && ch <= 'Z') {
          *keycode = KEY_A + (ch - 'A');
          *need_shift = true;
          return 0;
      }

      if (ch == ' ') { *keycode = KEY_SPACE; return 0; }
      if (ch == '\n' || ch == '\r') { *keycode = KEY_ENTER; return 0; }

      return -EINVAL;
  }
  ```

#### 2. `syscalls_metrics.c`

- **Propósito:**
  Proporciona métricas del sistema, como el uso de CPU y RAM, mediante una syscall personalizada.
- **Lógica relevante:**
  - Calcula el uso de CPU con `get_cpu_percent_x100` midiendo el delta de tiempos de CPU.
  - Calcula el uso de RAM basado en `MemAvailable` y `totalram`.
- **Fragmento de código:**

  ```c
  SYSCALL_DEFINE2(get_sys_usage, int __user *, cpu_x100, int __user *, ram_x100)
  {
      u32 cpu = get_cpu_percent_x100();
      u32 ram = get_ram_percent_x100();

      if (!cpu_x100 || !ram_x100)
          return -EINVAL;

      if (put_user((int)cpu, cpu_x100))
          return -EFAULT;
      if (put_user((int)ram, ram_x100))
          return -EFAULT;

      return 0;
  }
  ```

#### 3. `syscalls_usac1.c`

- **Propósito:**
  Implementa un mouse virtual que permite mover el cursor mediante una syscall personalizada.
- **Lógica relevante:**
  - Inicializa el mouse virtual con `init_usac_mouse`.
  - Reporta movimientos relativos del mouse con `input_report_rel`.
- **Fragmento de código:**

  ```c
  SYSCALL_DEFINE2(move_mouse, int, dx, int, dy)
  {
      mutex_lock(&mouse_mutex);

      if (!mouse) {
          int ret = init_usac_mouse();
          if (ret) {
              mutex_unlock(&mouse_mutex);
              return ret;
          }
      }

      input_report_rel(mouse, REL_X, dx);
      input_report_rel(mouse, REL_Y, dy);
      input_sync(mouse);

      mutex_unlock(&mouse_mutex);

      pr_info("Info: Mouse movido X=%d, Y=%d.\n", dx, dy);
      return 0;
  }
  ```

---
