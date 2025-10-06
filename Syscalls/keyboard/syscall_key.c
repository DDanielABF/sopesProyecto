#include <linux/syscalls.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/printk.h>

static struct input_dev *vkeyboard;
static DEFINE_MUTEX(vkeyboard_lock);

/* Inicializa el teclado virtual si aún no existe */
static int vkeyboard_ensure_created(void)
{
    int err;

    if (vkeyboard)
        return 0;

    mutex_lock(&vkeyboard_lock);
    if (vkeyboard) {
        mutex_unlock(&vkeyboard_lock);
        return 0;
    }

    vkeyboard = input_allocate_device();
    if (!vkeyboard) {
        mutex_unlock(&vkeyboard_lock);
        return -ENOMEM;
    }

    vkeyboard->name       = "virtual_keyboard_syscall";
    vkeyboard->phys       = "usb-virtual-keyboard/input0";
    vkeyboard->id.bustype = BUS_USB;
    vkeyboard->id.vendor  = 0x1234;
    vkeyboard->id.product = 0x5678;
    vkeyboard->id.version = 0x0001;

    /* El dispositivo soporta teclas (EV_KEY) */
    __set_bit(EV_KEY, vkeyboard->evbit);

    /* Habilitamos todas las teclas por simplicidad */
    for (int i = 0; i <= KEY_MAX; i++)
        __set_bit(i, vkeyboard->keybit);

    /* Y sincronización/repetición estándar */
    __set_bit(EV_REP, vkeyboard->evbit);

    err = input_register_device(vkeyboard);
    if (err) {
        input_free_device(vkeyboard);
        vkeyboard = NULL;
        mutex_unlock(&vkeyboard_lock);
        return err;
    }

    pr_info("vkeyboard(USB): dispositivo virtual registrado\n");
    mutex_unlock(&vkeyboard_lock);
    return 0;
}

/* --- Helpers de mapeo ASCII -> keycode + shift --- */

static int ascii_to_keycode(int ch, int *keycode, bool *need_shift)
{
    *need_shift = false;

    /* Letras a-z / A-Z */
    if (ch >= 'a' && ch <= 'z') {
        *keycode = KEY_A + (ch - 'a');
        return 0;
    }
    if (ch >= 'A' && ch <= 'Z') {
        *keycode = KEY_A + (ch - 'A');
        *need_shift = true;
        return 0;
    }

    /* Dígitos 0-9 */
    if (ch >= '1' && ch <= '9') {
        *keycode = KEY_1 + (ch - '1'); /* KEY_1..KEY_9 */
        return 0;
    }
    if (ch == '0') {
        *keycode = KEY_0;
        return 0;
    }

    /* Espacio, Enter, Tab y Backspace (útiles) */
    if (ch == ' ') { *keycode = KEY_SPACE; return 0; }
    if (ch == '\n' || ch == '\r') { *keycode = KEY_ENTER; return 0; }
    if (ch == '\t') { *keycode = KEY_TAB; return 0; }
    if (ch == 0x08) { *keycode = KEY_BACKSPACE; return 0; }

    /* Si quieres añadir más símbolos (.,,,;:-_+*/ etc.),
       mapea aquí cada caso y marca need_shift cuando aplique. */

    return -EINVAL; /* No soportado */
}

/* Emite una tecla (down/up), con opción de SHIFT previo */
static void vkbd_emit_key(int keycode, bool with_shift)
{
    if (with_shift) {
        input_report_key(vkeyboard, KEY_LEFTSHIFT, 1);
        input_sync(vkeyboard);
    }

    input_report_key(vkeyboard, keycode, 1);
    input_sync(vkeyboard);
    input_report_key(vkeyboard, keycode, 0);
    input_sync(vkeyboard);

    if (with_shift) {
        input_report_key(vkeyboard, KEY_LEFTSHIFT, 0);
        input_sync(vkeyboard);
    }
}

/* Syscall original: simula la pulsación de un keycode “crudo” */
SYSCALL_DEFINE1(send_key_event, int, keycode)
{
    int err = vkeyboard_ensure_created();
    if (err)
        return err;
    if (!vkeyboard)
        return -ENODEV;

    if (keycode < 0 || keycode > KEY_MAX)
        return -EINVAL;

    vkbd_emit_key(keycode, false);
    pr_info("sys_send_key_event: keycode=%d\n", keycode);
    return 0;
}

/* Nueva syscall: acepta un carácter ASCII alfanumérico (y algunos básicos) */
SYSCALL_DEFINE1(send_char_event, int, ch)
{
    int err, keycode;
    bool need_shift;

    err = vkeyboard_ensure_created();
    if (err)
        return err;
    if (!vkeyboard)
        return -ENODEV;

    if (ch < 0 || ch > 255)
        return -EINVAL;

    err = ascii_to_keycode(ch, &keycode, &need_shift);
    if (err)
        return err; /* -EINVAL si no soportado */

    vkbd_emit_key(keycode, need_shift);
    pr_info("sys_send_char_event: ch=%d ('%c') -> keycode=%d shift=%d\n",
            ch, (ch >= 32 && ch < 127) ? ch : '?', keycode, need_shift);
    return 0;
}
