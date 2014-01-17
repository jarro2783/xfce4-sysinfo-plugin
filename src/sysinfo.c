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

  double** history;
  size_t history_start;
  size_t history_end;
  size_t history_size;

  //minimum and maximums of all the data preceding
  double* history_min;
  double* history_max;

  //which plugin are we handling
  SysinfoPlugin* plugin;

  //our frame
  GtkWidget* frame;
  GtkWidget* drawing;
} FrameData;

struct sysinfoinstance
{
  XfcePanelPlugin* plugin;
  SysinfoPluginList* plugin_list;
  GtkWidget* top;
  GtkWidget* hvbox;

  size_t num_displayed;
  FrameData* drawn_frames;

  guint timeout_id;
};

static gboolean
draw_graph_cb(GtkWidget* w, GdkEventExpose* event, FrameData* frame)
{
  cairo_t* cr = gdk_cairo_create(w->window);

  gint width = w->allocation.width;
  gint height = w->allocation.height;

  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, 0.2, 0.2, 1);

  //compute where the zero is for this frame

  frame->plugin->

  //draw each data point as a line from 0 to the value
  //the drawing area needs to be scaled appropriately

#if 0
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
#endif

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

  sysinfo->drawn_frames = g_new0(FrameData, num_plugins);

  fprintf(stderr, "opened %zu plugins\n", num_plugins);

  while (i != num_plugins)
  {
    FrameData* fd = &sysinfo->drawn_frames[i];

    GtkWidget* frame = gtk_frame_new(NULL);
    GtkWidget* drawing = gtk_drawing_area_new();
    //GtkWidget* drawing = gtk_label_new("Hello world");

    gtk_container_add(GTK_CONTAINER(frame), drawing);
    gtk_box_pack_end(GTK_BOX(sysinfo->hvbox), frame, TRUE, TRUE, 0);

    fd->frame = frame;
    fd->drawing = drawing;

    g_signal_connect_after(G_OBJECT(drawing), "expose-event",
                     G_CALLBACK(draw_graph_cb), fd);

    SysinfoPlugin* plugin = sysinfo_pluginlist_get(sysinfo->plugin_list, i);
    fd->plugin = plugin;
    fd->history = g_new0(double*, plugin->num_data);

    ++i;
  }

  gtk_widget_show_all(sysinfo->top);
}

static void
update_history(FrameData* frame, int fields, double* data)
{
  //add a new data point into the history
  //computes the new min and max

  //TODO min and max
  
  //slide the window along one

  //move the end, if it goes past the end, wrap back to the beginning
  //then increment the start if they overlap
  //if the start goes past the end then wrap back to the beginning
  ++frame->history_end;

  if (frame->history_end == frame->history_size)
  {
    frame->history_end = 0;
  }

  if (frame->history_end == frame->history_start)
  {
    ++frame->history_start;
  }

  if (frame->history_start == frame->history_end)
  {
    frame->history_start = 0;
  }

  size_t i = 0;
  while (i != fields)
  {
    frame->history[i][frame->history_end] = data[i];
    ++i;
  }
}

static void
update_frame(FrameData* frame)
{
  SysinfoPluginData data;
  (*frame->plugin->get_data)(frame->plugin, &data);

  update_history(frame, frame->plugin->num_data, data.data);
}

static gboolean
update(SysinfoInstance* sysinfo)
{
  size_t i = 0;
  while (i != sysinfo->num_displayed)
  {
    update_frame(&sysinfo->drawn_frames[i]);
    gtk_widget_queue_draw(sysinfo->drawn_frames[i].drawing);
    ++i;
  }

  return TRUE;
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

  sysinfo->timeout_id = g_timeout_add(250, (GSourceFunc)update, sysinfo);

  //do one update
  update(sysinfo);

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
    gtk_widget_set_size_request (sysinfo->drawn_frames[i].frame, w, h);
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
