#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/input.h>
#include <linux/mutex.h>
#include <linux/errno.h>

static struct input_dev *mouse = NULL;
static DEFINE_MUTEX(mouse_mutex);

static int init_usac_mouse(void)
{
    int err;

    if (mouse)
        return 0;

    mouse = input_allocate_device();
    if (!mouse) {
        pr_err("Error: No se pudo alocar el dispositivo de mouse virtual.\n");
        return -ENOMEM;
    }

    mouse->name = "Mouse Virtual ";
    mouse->phys = "virtual_mouse";
    mouse->id.bustype = BUS_VIRTUAL;
    mouse->id.vendor = 0x1947;
    mouse->id.product = 0x0011;
    mouse->id.version = 0x0001;

    __set_bit(EV_REL, mouse->evbit);
    __set_bit(REL_X, mouse->relbit);
    __set_bit(REL_Y, mouse->relbit);
    __set_bit(EV_KEY, mouse->evbit);
    __set_bit(BTN_LEFT, mouse->keybit);

    err = input_register_device(mouse);
    if (err) {
        pr_err("Error: No se pudo registrar el mouse virtual: %d.\n", err);
        input_free_device(mouse);
        mouse = NULL;
        return err;
    }

    pr_info("Info: Mouse virtual inicializado correctamente.\n");
    return 0;
}

SYSCALL_DEFINE2(move_mouse, int, dx, int, dy)
{
    int ret = 0;

    mutex_lock(&mouse_mutex);

    if (!mouse) {
        ret = init_usac_mouse();
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