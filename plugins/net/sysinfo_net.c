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
#include <glibtop/netload.h>
#include <math.h>

#include "xfce4-sysinfo-plugin/plugins.h"

#define DATA_FIELDS 3

static double logscale;

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

  //the actual data this and list slice
  double* current;
  double* last;

  //the computed data rate
  double* rate;
} NetData;

static void
init_color(SysinfoPlugin* plugin)
{
  SysinfoColor* c = g_new(SysinfoColor, DATA_FIELDS);

  c[NET_OUT].red = 0xed / 255.;
  c[NET_OUT].green = 0xd4 / 255.;
  c[NET_OUT].blue = 0x00 / 255.;

  c[NET_IN].red = 0xfc / 255.;
  c[NET_IN].green = 0xe9 / 255.;
  c[NET_IN].blue = 0x4f / 255.;

  c[NET_LOCAL].red = 0xc4 / 255.;
  c[NET_LOCAL].green = 0xa0 / 255.;
  c[NET_LOCAL].blue = 0;

  plugin->colors = c;
}

static void
add_data(double* result, double value)
{
  *result = *result + value;
}

static void
collect_data(NetData* data)
{
  //swap buffers around
  double* current = data->last;
  double* last = data->last = data->current;
  data->current = current;

  //zero the current data
  memset(current, 0, sizeof(double) * DATA_FIELDS);

  size_t i = 0;
  while (i != data->num_interfaces)
  {
    glibtop_netload buf;
    glibtop_get_netload(&buf, data->interfaces[i]);

    if (buf.if_flags & (1L << GLIBTOP_IF_FLAGS_LOOPBACK))
    {
      //add all data to local data collection
      add_data(&current[NET_LOCAL], buf.bytes_total);
    }
    else
    {
      add_data(&current[NET_IN], buf.bytes_in);
      add_data(&current[NET_OUT], buf.bytes_out);
    }

    ++i;
  }

  //compute the rates
  i = 0;
  while (i != DATA_FIELDS)
  {
    data->rate[i] = current[i] - last[i];
    ++i;
  }
}

static void
get_range(double min, double max, double* display_min, double* display_max)
{
  //scale based on the max
  //for max = x.y E z, we go to x + 1 E z

  *display_min = 0;

  *display_max = pow(1.8, ceil(logscale * log(max)));
}

static void
get_data(SysinfoPlugin* plugin, SysinfoPluginData* data)
{
  NetData* plugin_data = (NetData*)plugin->plugin_data;

  collect_data(plugin_data);

  data->data = plugin_data->rate;
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

  g_free(data->current);
  g_free(data->last);
  g_free(data->rate);
}

static NetData*
init_data()
{
  NetData* data = g_new(NetData, 1);

  data->current = g_new(double, DATA_FIELDS);
  data->last = g_new(double, DATA_FIELDS);
  data->rate = g_new(double, DATA_FIELDS);

  get_interfaces(data);

  //do a first run so that we have some meaningful numbers for the next
  //time around
  collect_data(data);

  return data;
}

SysinfoPlugin*
sysinfo_data_plugin_init()
{
  //repetition if the plugin is loaded up multiple times, but it's really
  //quite irrelevant
  logscale = 1/log(1.8);

  SysinfoPlugin* plugin = g_new(SysinfoPlugin, 1);

  plugin->plugin_name = "Net Load";
  plugin->num_data = DATA_FIELDS;
  plugin->data_names = data_names;

  init_color(plugin);

  plugin->plugin_data = init_data();

  plugin->close = &close;
  plugin->get_data = &get_data;
  plugin->get_range = &get_range;

  return plugin;
}
