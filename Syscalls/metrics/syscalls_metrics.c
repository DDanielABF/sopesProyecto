// kernel/sys_sys_usage.c
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/vmstat.h>
#include <linux/delay.h>
#include <linux/kernel_stat.h>   // kcpustat_cpu()
#include <linux/sched/cputime.h> // CPUTIME_*
#include <linux/cpumask.h>

static void read_cpu_times(u64 *idle, u64 *total)
{
    int cpu;
    u64 user = 0, nice = 0, system = 0, irq = 0, softirq = 0, steal = 0, iowait = 0, idle_t = 0;

    for_each_online_cpu(cpu) {
        const struct kernel_cpustat *kcs = &kcpustat_cpu(cpu);
        user    += kcs->cpustat[CPUTIME_USER];
        nice    += kcs->cpustat[CPUTIME_NICE];
        system  += kcs->cpustat[CPUTIME_SYSTEM];
        irq     += kcs->cpustat[CPUTIME_IRQ];
        softirq += kcs->cpustat[CPUTIME_SOFTIRQ];
        steal   += kcs->cpustat[CPUTIME_STEAL];
        iowait  += kcs->cpustat[CPUTIME_IOWAIT];
        idle_t  += kcs->cpustat[CPUTIME_IDLE];
    }

    *idle  = idle_t;
    *total = user + nice + system + irq + softirq + steal + iowait + idle_t;
}

// Retorna CPU ocupada en centésimas de % (p. ej., 5321 => 53.21%)
static u32 get_cpu_percent_x100(void)
{
    u64 idle1, total1, idle2, total2;
    s64 didle, dtotal;

    read_cpu_times(&idle1, &total1);
    msleep(100); // pequeño intervalo para medir delta; puedes ajustar/evitar (ver nota abajo)
    read_cpu_times(&idle2, &total2);

    didle  = (s64)idle2  - (s64)idle1;
    dtotal = (s64)total2 - (s64)total1;
    if (dtotal <= 0)
        return 0;

    return (u32)div64_u64((u64)(dtotal - didle) * 10000ULL, (u64)dtotal);
}

// Retorna RAM usada (basado en MemAvailable) en centésimas de %
static u32 get_ram_percent_x100(void)
{
    struct sysinfo i;
    unsigned long avail, total;

    si_meminfo(&i);                // totalram, freeram, etc., en páginas
    avail = si_mem_available();    // páginas “usables” disponibles
    total = i.totalram;

    if (total == 0)
        return 0;

    return (u32)div64_u64((u64)(total - avail) * 10000ULL, (u64)total);
}

// Syscall: escribe en *cpu_x100 y *ram_x100 los porcentajes en centésimas
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
