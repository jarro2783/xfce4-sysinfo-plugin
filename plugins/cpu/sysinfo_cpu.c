/* cpu data for xfce4-sysinfo-plugin

   Copyright (C) 2013 Jarryd Beck

This file is part of xfce4-sysinfo-plugin.

xfce4-sysinfo-plugin is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

xfce4-sysinfo-plugin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with xfce4-sysinfo-plugin; see the file COPYING.  If not see
<http://www.gnu.org/licenses/>.  

*/

#include <glibtop/cpu.h>

#include "xfce4-sysinfo-plugin/plugins.h"

#define NUM_FIELDS 4

enum
{
  CPU_TOTAL,
  CPU_NICE,
  CPU_SYS,
  CPU_IOWAIT
};

static gchar* data_names[NUM_FIELDS] =
  {
    "Total",
    "Nice",
    "System",
    "IO Wait"
  };

void
cpu_get_data(SysinfoPlugin* plugin, SysinfoPluginData* data)
{
  glibtop_cpu cpu;
  glibtop_get_cpu(&cpu);

  //always has 0 to 100%
  data->lower = 0;
  data->upper = 100;

  //the_data[CPU_TOTAL] = cpu.total;
}

static void
cpu_close(SysinfoPlugin* plugin)
{
}

SysinfoPlugin*
sysinfo_data_plugin_init()
{
  SysinfoPlugin* plugin = g_new(SysinfoPlugin, 1);

  plugin->num_data = NUM_FIELDS;
  plugin->data_names = data_names;
  plugin->plugin_data = g_new(double, NUM_FIELDS);

  plugin->get_data = &cpu_get_data;
  plugin->close = &cpu_close;
}
