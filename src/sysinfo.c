/* xfce4-sysinfo-plugin
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
<http://www.gnu.org/licenses/>.  */

#include "xfce4-sysinfo-plugin/xfce4_sysinfo_plugin.h"
#include "xfce4-sysinfo-plugin/plugins.h"

#include <libxfce4panel/xfce-panel-plugin.h>

typedef struct
{
  XfcePanelPlugin* plugin;
  GtkWidget* top;
} SysinfoInstance;

static void
draw_graph_cb(GtkWidget* w, GdkEventExpose* event, gpointer data)
{
}

static void
construct_gui(XfcePanelPlugin* plugin, SysinfoInstance* sysinfo)
{
  GtkOrientation orientation = xfce_panel_plugin_get_orientation(plugin);

  sysinfo->top = xfce_hvbox_new(orientation, FALSE, 0);
}

static SysinfoInstance*
sysinfo_construct(XfcePanelPlugin* plugin)
{
  sysinfo_initialise();

  SysinfoInstance* sysinfo;

  sysinfo = g_slice_new0(SysinfoInstance);

  sysinfo->plugin = plugin;

  construct_gui(plugin, sysinfo);

  return sysinfo;
}

static void
sysinfo_free(SysinfoInstance* sysinfo)
{
  g_slice_free(SysinfoInstance, sysinfo);

  glibtop_close();
}

static void
sysinfo_init(XfcePanelPlugin* plugin)
{
  //create new plugin
  SysinfoInstance* sysinfo = sysinfo_construct(plugin);

  //load up the sysinfo data source plugins
  SysinfoPluginList* list = sysinfo_load_plugins();

  //connect some signals
  g_signal_connect (G_OBJECT(plugin), "free-data",
                    G_CALLBACK (sysinfo_free), sysinfo);
}

XFCE_PANEL_PLUGIN_REGISTER(sysinfo_init);
