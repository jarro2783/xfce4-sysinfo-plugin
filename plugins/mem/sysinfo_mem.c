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

#define DATA_FIELDS 4
#define TOOLTIP_SIZE 300

enum
{
  MEM_FREE,
  MEM_BUFFERS,
  MEM_CACHED,
  MEM_USER
};

static gchar* data_names[DATA_FIELDS] =
  {
    "Free",
    "Buffers",
    "Cached",
    "User"
  };

typedef struct
{
  double total;
  double* data;

  gchar tooltip[TOOLTIP_SIZE];
} MemData;

static void
get_data(MemData* d)
{
  glibtop_mem buf;
  glibtop_get_mem(&buf);

  d->data[MEM_BUFFERS] = buf.buffer;
  d->data[MEM_CACHED] = buf.cached;
  d->data[MEM_USER] = buf.user;

  d->total = buf.total;
}

static void
mem_get_data(SysinfoPlugin* plugin, SysinfoPluginData* data)
{
  MemData* mem = (MemData*)plugin->plugin_data;

  get_data(mem);

  data->data = mem->data;
}

static void
mem_close(SysinfoPlugin* plugin)
{
  MemData* data = (MemData*)plugin->plugin_data;

  g_free(data->data);
  //g_free(data->tooltip);
  g_free(data);

  g_free(plugin->colors);

  g_free(plugin);
}

static void
mem_get_range
(
  SysinfoPlugin* plugin,
  double min, 
  double max, 
  double* display_min, 
  double* display_max
)
{
  MemData* data = (MemData*)plugin->plugin_data;

  //always 0 to total
  *display_min = 0;
  *display_max = data->total;
}

static gchar*
mem_get_tooltip(SysinfoPlugin* plugin)
{
  MemData* data = (MemData*)plugin->plugin_data;
  gchar* t = data->tooltip;

  gchar* user = g_format_size(data->data[MEM_USER]);
  gchar* buffer = g_format_size(data->data[MEM_BUFFERS]);
  gchar* cached = g_format_size(data->data[MEM_CACHED]);
  gchar* total = g_format_size(data->total);

  g_snprintf(
    t,
    TOOLTIP_SIZE,
    "== Memory Usage ==\n\nTotal: %s\nUser: %s\nBuffers: %s\nCached: %s",
    total, user, buffer, cached
  );

  g_free(user);
  g_free(buffer);
  g_free(cached);
  g_free(total);

  return t;
}

static void
mem_reset_colors(SysinfoPlugin* plugin)
{
  SysinfoColor* c = plugin->colors;

  c[MEM_FREE].red = 0;
  c[MEM_FREE].green = 0;
  c[MEM_FREE].blue = 0;

  c[MEM_USER].red = 0x00 / 255.;
  c[MEM_USER].green = 0xb3 / 255.;
  c[MEM_USER].blue = 0x3b / 255.;

  c[MEM_BUFFERS].red = 0x00 / 255.;
  c[MEM_BUFFERS].green = 0xff / 255.;
  c[MEM_BUFFERS].blue = 0x82 / 255.;

  c[MEM_CACHED].red = 0xaa / 255.;
  c[MEM_CACHED].green = 0xf5 / 255.;
  c[MEM_CACHED].blue = 0xd0 / 255.;
}

static void
init_color(SysinfoPlugin* plugin)
{
  SysinfoColor* c = g_new0(SysinfoColor, DATA_FIELDS);

  plugin->colors = c;

  mem_reset_colors(plugin);
}

SysinfoPlugin*
sysinfo_data_plugin_init()
{
  SysinfoPlugin* plugin = g_new0(SysinfoPlugin, 1);

  plugin->plugin_name = "Memory Usage";
  plugin->num_data = DATA_FIELDS;
  plugin->data_names = data_names;

  init_color(plugin);

  MemData* data = g_new(MemData, 1);
  data->data = g_new(double, DATA_FIELDS);

  get_data(data);

  plugin->plugin_data = data;

  plugin->get_data = &mem_get_data;
  plugin->close = &mem_close;
  plugin->get_range = &mem_get_range;
  plugin->reset_colors = &mem_reset_colors;
  plugin->get_tooltip = &mem_get_tooltip;

  return plugin;
}
