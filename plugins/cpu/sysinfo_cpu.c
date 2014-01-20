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
#include <math.h>

#include "xfce4-sysinfo-plugin/plugins.h"

#define DATA_FIELDS 4
#define RECORD_FIELDS 5

enum
{
  CPU_NICE,
  CPU_SYS,
  CPU_IOWAIT,
  CPU_USER,
  CPU_TOTAL
};

static gchar* data_names[DATA_FIELDS] =
  {
    "Nice",
    "System",
    "IO Wait"
    "User",
  };

typedef struct
{
  double* last;
  double* current;
  double* percentages;
} CPUData;

static void
get_data(double* data)
{
  glibtop_cpu cpu;
  glibtop_get_cpu(&cpu);

  data[CPU_USER] = cpu.user;
  data[CPU_NICE] = cpu.nice;
  data[CPU_SYS] = cpu.sys;
  data[CPU_IOWAIT] = cpu.iowait;
  data[CPU_TOTAL] = cpu.total;
}

void
cpu_get_data(SysinfoPlugin* plugin, SysinfoPluginData* data)
{
  CPUData* the_data = (CPUData*)plugin->plugin_data;

  //swap the buffers
  double* current = the_data->last;
  double* last = the_data->current;
  the_data->last = last;
  the_data->current = current;

  get_data(current);

  //the percentage is the difference between the current and last divided
  //by the difference in the total

  double totaldelta = 
    current[CPU_TOTAL] - last[CPU_TOTAL];

  double* p = the_data->percentages;
  p[CPU_USER] = (current[CPU_USER] - last[CPU_USER]) / totaldelta;
  p[CPU_NICE] = (current[CPU_NICE] - last[CPU_NICE]) / totaldelta;
  p[CPU_SYS] = (current[CPU_SYS] - last[CPU_SYS]) / totaldelta;
  p[CPU_IOWAIT] = (current[CPU_IOWAIT] - last[CPU_IOWAIT]) / totaldelta;

  //multiply to percentage and round
  size_t i = 0;
  while (i != DATA_FIELDS)
  {
    p[i] = round(p[i] * 100);
    ++i;
  }

  data->data = p;
}

static void 
cpu_get_range
(
  double min, 
  double max, 
  double* display_min, 
  double* display_max
)
{
  //always display 0 to 100
  *display_min = 0;
  *display_max = 100;
}

static void
cpu_close(SysinfoPlugin* plugin)
{
}

static void
init_color(SysinfoPlugin* plugin)
{
  SysinfoColor* c = g_new(SysinfoColor, DATA_FIELDS);

  c[CPU_NICE].red = 0x66 / 255.;
  c[CPU_NICE].green = 0xa3 / 255.;
  c[CPU_NICE].blue = 0xd2 / 255.;

  c[CPU_SYS].red = 0x3f / 255.;
  c[CPU_SYS].green = 0x92 / 255.;
  c[CPU_SYS].blue = 0xd2 / 255.;

  c[CPU_IOWAIT].red = 0x0b / 255.;
  c[CPU_IOWAIT].green = 0x61 / 255.;
  c[CPU_IOWAIT].blue = 0xa4 / 255.;

  c[CPU_USER].red = 0x03 / 255.;
  c[CPU_USER].green = 0x3e / 255.;
  c[CPU_USER].blue = 0x6b / 255.;

  plugin->colors = c;
}

SysinfoPlugin*
sysinfo_data_plugin_init()
{
  SysinfoPlugin* plugin = g_new(SysinfoPlugin, 1);

  plugin->plugin_name = "CPU Usage";
  plugin->num_data = DATA_FIELDS;
  plugin->data_names = data_names;

  init_color(plugin);

  //initialise the stats buffers
  CPUData* data = g_new(CPUData, 1);
  data->last = g_new(double, RECORD_FIELDS);
  data->current = g_new(double, RECORD_FIELDS);
  data->percentages = g_new(double, DATA_FIELDS);

  //get some data to start from
  //put it in current because next round will swap it to last
  get_data(data->current);

  plugin->plugin_data = data;

  plugin->get_data = &cpu_get_data;
  plugin->get_range = &cpu_get_range;
  plugin->close = &cpu_close;

  return plugin;
}
