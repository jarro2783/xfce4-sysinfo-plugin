#include <glibtop/cpu.h>

void
sysinfo_data_plugin_init()
{
}

void
cpu_get_data()
{
  glibtop_cpu cpu;
  glibtop_get_cpu(&cpu);
}

static void
cpu_close()
{
}
