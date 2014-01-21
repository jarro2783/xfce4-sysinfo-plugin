/* net data for xfce4-sysinfo-plugin

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

#include <glibtop/netlist.h>
#include <math.h>

#include "xfce4-sysinfo-plugin/plugins.h"

#define DATA_FIELDS 3

enum
{
  NET_IN,
  NET_OUT,
  NET_LOCAL
};

static gchar* data_names[DATA_FIELDS] =
{
  "In",
  "Out",
  "Local"
};

typedef struct
{
  gchar** interfaces;
  int num_interfaces;
} NetData;

static void
init_color(SysinfoPlugin* plugin)
{
  SysinfoColor* c = g_new(SysinfoColor, DATA_FIELDS);

  c[NET_IN].red = 0xff / 255.;
  c[NET_IN].green = 0xe8 / 255.;
  c[NET_IN].blue = 0;

  c[NET_OUT].red = 0xff / 255.;
  c[NET_OUT].green = 0xf2 / 255.;
  c[NET_OUT].blue = 0x73 / 255.;

  c[NET_LOCAL].red = 0xa6 / 255.;
  c[NET_LOCAL].green = 0x97 / 255.;
  c[NET_LOCAL].blue = 0;

  plugin->colors = c;
}

static void
get_interfaces(NetData* data)
{
  glibtop_netlist list;
  data->interfaces = glibtop_get_netlist(&list);
  data->num_interfaces = list.number;
}

static void
close(SysinfoPlugin* plugin)
{
  NetData* data = (NetData*)plugin->plugin_data;

  g_strfreev(data->interfaces);
}

SysinfoPlugin*
sysinfo_data_plugin_init()
{
  SysinfoPlugin* plugin = g_new(SysinfoPlugin, 1);

  plugin->plugin_name = "Net Load";
  plugin->num_data = DATA_FIELDS;
  plugin->data_names = data_names;

  init_color(plugin);

  NetData* data = g_new(NetData, 1);
  plugin->plugin_data = data;

  return plugin;
}
