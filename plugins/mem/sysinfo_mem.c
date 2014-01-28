/* memory data for xfce4-sysinfo-plugin

   Copyright (C) 2014 Jarryd Beck

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

#include <glibtop/mem.h>

#include "xfce4-sysinfo-plugin/plugins.h"

#define DATA_FIELDS 3

enum
{
  MEM_BUFFERS,
  MEM_CACHED,
  MEM_USER
};

static gchar* data_names[DATA_FIELDS] =
  {
    "Buffers",
    "Cached",
    "User"
  };

typedef struct
{
  double total;
  double* data;
} MemData;

static void
get_data(MemData* d)
{
  glibtop_mem buf;
  glibtop_get_mem(&buf);
}

static void
mem_get_data(SysinfoPlugin* plugin, SysinfoPluginData* data)
{
  MemData* mem = (MemData*)plugin->plugin_data;
}

SysinfoPlugin*
sysinfo_data_plugin_init()
{
  SysinfoPlugin* plugin = g_new0(SysinfoPlugin, 1);

  plugin->plugin_name = "Memory Usage";
  plugin->num_data = DATA_FIELDS;
  plugin->data_names = data_names;

  MemData* data = g_new(MemData, 1);
  data->data = g_new(double, DATA_FIELDS);

  plugin->plugin_data = data;

  plugin->get_data = &mem_get_data;

  return plugin;
}
