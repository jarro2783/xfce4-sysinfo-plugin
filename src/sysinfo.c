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

typedef struct sysinfoinstance SysinfoInstance;

typedef struct
{
  SysinfoInstance* sysinfo;
  
  //which frame are we on?
  size_t frame;
} FrameData;

struct sysinfoinstance
{
  XfcePanelPlugin* plugin;
  SysinfoPluginList* plugin_list;
  GtkWidget* top;
  GtkWidget* hvbox;

  size_t num_displayed;
  GtkWidget** frames;
  GtkWidget** drawing;

  FrameData* drawn_frames;
};

static gboolean
draw_graph_cb(GtkWidget* w, GdkEventExpose* event, SysinfoInstance* sysinfo)
{
  cairo_t* cr = gdk_cairo_create(w->window);

  gint width = w->allocation.width;
  gint height = w->allocation.height;

  cairo_set_line_width(cr, 1);
  cairo_move_to(cr, width, height);
  cairo_line_to(cr, width, height - 20);
  cairo_move_to(cr, width - 1, height);
  cairo_line_to(cr, width - 1, 22);
  cairo_move_to(cr, width - 2, height);
  cairo_line_to(cr, width - 2, 24);
  cairo_move_to(cr, width - 3, height);
  cairo_line_to(cr, width - 3, 26);
  cairo_stroke(cr);
  return TRUE;
}

static gboolean
orientation_cb
(
  XfcePanelPlugin* plugin, 
  GtkOrientation o, 
  SysinfoInstance* sysinfo
)
{
  xfce_hvbox_set_orientation(XFCE_HVBOX(sysinfo->hvbox), o);

  return FALSE;
}

static void
construct_gui(XfcePanelPlugin* plugin, SysinfoInstance* sysinfo)
{
  GtkOrientation orientation = xfce_panel_plugin_get_orientation(plugin);

  sysinfo->top = gtk_event_box_new();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(sysinfo->top), FALSE);
  gtk_event_box_set_above_child(GTK_EVENT_BOX(sysinfo->top), TRUE);

  xfce_panel_plugin_add_action_widget(plugin, sysinfo->top);

  gtk_container_add(GTK_CONTAINER(plugin), sysinfo->top);

  sysinfo->hvbox = xfce_hvbox_new(orientation, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(sysinfo->top), sysinfo->hvbox);

  //each graph is a drawing area in a frame
  size_t num_plugins = sysinfo_pluginlist_size(sysinfo->plugin_list);
  size_t i = 0;

  sysinfo->num_displayed = num_plugins;

  fprintf(stderr, "opened %zu plugins\n", num_plugins);

  sysinfo->frames = g_new(GtkWidget*, num_plugins);
  sysinfo->drawing = g_new(GtkWidget*, num_plugins);

  while (i != num_plugins)
  {
    GtkWidget* frame = gtk_frame_new(NULL);
    GtkWidget* drawing = gtk_drawing_area_new();
    //GtkWidget* drawing = gtk_label_new("Hello world");

    gtk_container_add(GTK_CONTAINER(frame), drawing);
    gtk_box_pack_end(GTK_BOX(sysinfo->hvbox), frame, TRUE, TRUE, 0);

    sysinfo->frames[i] = frame;
    sysinfo->drawing[i] = drawing;
    
    g_signal_connect_after(G_OBJECT(drawing), "expose-event",
                     G_CALLBACK(draw_graph_cb), sysinfo);

    ++i;
  }

  gtk_widget_show_all(sysinfo->top);
}

static SysinfoInstance*
sysinfo_construct(XfcePanelPlugin* plugin)
{
  sysinfo_initialise();

  SysinfoInstance* sysinfo;

  sysinfo = g_slice_new0(SysinfoInstance);

  sysinfo->plugin = plugin;

  sysinfo->plugin_list = sysinfo_load_plugins();

  construct_gui(plugin, sysinfo);

  return sysinfo;
}

static gboolean
size_changed_cb
(
  XfcePanelPlugin* plugin,
  gint size,
  SysinfoInstance* sysinfo
)
{
  GtkOrientation orientation;

  fprintf(stderr, "size changed to %d\n", size);

  /* get the orientation of the plugin */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  gint h, w;

  /* set the widget size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
  {
    h = size;
    w = 60;
  }
  else
  {
    w = size;
    h = 60;
  }
  
  size_t i = 0;
  while (i != sysinfo->num_displayed)
  {
    //gtk_widget_set_size_request (GTK_WIDGET(plugin), w, h);
    gtk_widget_set_size_request (sysinfo->frames[i], w, h);
    //gtk_widget_set_size_request (sysinfo->drawing[i], w, h);
    ++i;
  }

  /* we handled the orientation */
  return TRUE;
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
  g_signal_connect (G_OBJECT(plugin), "size-changed",
                    G_CALLBACK (size_changed_cb), sysinfo);
  g_signal_connect (G_OBJECT(plugin), "orientation-changed", 
                    G_CALLBACK(orientation_cb), sysinfo);

}

XFCE_PANEL_PLUGIN_REGISTER(sysinfo_init);
