/* xfce4-sysinfo-plugin
   Copyright (C) 2013, 2014 Jarryd Beck

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

#include <glibtop/close.h>

#define DEFAULT_HISTORY_SIZE 80
#define DEFAULT_WIDTH 60

typedef struct sysinfoinstance SysinfoInstance;

typedef struct
{
  SysinfoInstance* sysinfo;

  double** history;
  int history_start;
  int history_end;
  int history_size;
  double* history_sum;

  double history_max;

  //which plugin are we handling
  SysinfoPlugin* plugin;

  gint width;

  //our frame
  GtkWidget* frame;
  GtkWidget* drawing;
  GtkWidget* tooltip_text;

  //maximum of sliding window data
  GQueue* slidingqueue;
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

static inline int
draw_one_point
(
  cairo_t* cr, 
  SysinfoColor* color,
  double scale,
  int base, 
  gint width, 
  gint height, 
  int x, 
  double y
)
{
  int xdraw = x;
  int ydraw = base - scale * y;

  cairo_set_source_rgb(cr, 
    color->red, 
    color->green,
    color->blue
  );

  cairo_move_to(cr, xdraw + 0.5, base + 0.5);
  cairo_line_to(cr, xdraw + 0.5, ydraw + 0.5);

  cairo_stroke(cr);
  
  return ydraw;
}

static gboolean
draw_graph_cb(GtkWidget* w, GdkEventExpose* event, FrameData* frame)
{
  cairo_t* cr = gdk_cairo_create(w->window);

  SysinfoPlugin* plugin = frame->plugin;

  gint width = frame->width;
  gint height = w->allocation.height;

  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill(cr);

  cairo_set_line_width(cr, 1);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

  //compute where the zero is for this frame

  double min = 0;
  double max = 100;
  frame->plugin->get_range(0, frame->history_max, &min, &max);

  double range = max - min;
  double scale = height / range;

  int base = height - scale * min;

  //draw each data point as a line from 0 to the value
  //the drawing area needs to be scaled appropriately

  int slices =
    (frame->history_end - frame->history_start);
  slices = slices <= 0 ? slices + frame->history_size : slices;
  int num_items = width < slices ? width : slices;

  int i = (frame->history_end - num_items);
  i = i < 0 ? i + frame->history_size : i;

  int nextbase = base;
  int x = width - num_items;
  while (i != frame->history_end && i != frame->history_size)
  {
    nextbase = base;
    size_t j = 0;
    while (j != frame->plugin->num_data)
    {
      //draw
      nextbase = draw_one_point(cr, &plugin->colors[j], scale, nextbase, 
        width, height, x, frame->history[j][i]);

      ++j;
    }

    ++i;
    ++x;
  }

  if (i == frame->history_size)
  {
    i = 0;

    while (i != frame->history_end)
    {
      nextbase = base;
      size_t j = 0;
      while (j != frame->plugin->num_data)
      {
        //draw
        nextbase = draw_one_point(cr, &plugin->colors[j],
          scale, nextbase, width, height, x, frame->history[j][i]);
        ++j;
      }

      ++i;
      ++x;
    }
  }

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
update_tooltip(FrameData* fd)
{
  gchar* text = (*fd->plugin->get_tooltip)(fd->plugin);

  gtk_label_set_text(GTK_LABEL(fd->tooltip_text), text);
}

static gboolean
tooltip_cb
(
  GtkWidget* widget,
  gint x,
  gint y,
  gboolean kb_mode,
  GtkTooltip* tooltip,
  gpointer data
)
{
  FrameData* fd = (FrameData*)data;

  update_tooltip(fd);

  gtk_tooltip_set_custom(tooltip, fd->tooltip_text);

  return TRUE;
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
    fd->history_sum = g_new0(double, DEFAULT_HISTORY_SIZE);

    fd->history_size = DEFAULT_HISTORY_SIZE;
    fd->history_end = 0;
    fd->history_start = 0;

    //allocate a history for each component of the data
    size_t j = 0;
    while (j != plugin->num_data)
    {
      //allocate something so that we have a valid history
      fd->history[j] = g_new0(double, DEFAULT_HISTORY_SIZE);
      fd->width = DEFAULT_WIDTH;
      fd->slidingqueue = g_queue_new();
      ++j;
    }

    gtk_widget_set_has_tooltip(drawing, TRUE);
    fd->tooltip_text = gtk_label_new(NULL);
    g_object_ref(fd->tooltip_text);
    g_signal_connect(drawing, "query-tooltip", G_CALLBACK(tooltip_cb), fd);

    ++i;
  }

  gtk_widget_show_all(sysinfo->top);
}

static void
update_history(FrameData* frame, int fields, double* data)
{
  //add a new data point into the history
  //computes the new min and max

  //slide the window along one

  //fill the data first
  size_t i = 0;

  int sum = 0;
  while (i != fields)
  {
    frame->history[i][frame->history_end] = data[i];
    sum += data[i];
    ++i;
  }

  GQueue* q = frame->slidingqueue;

  //add the new value into the maximum
  while (!g_queue_is_empty(q) && 
    sum >= frame->history_sum[GPOINTER_TO_INT(g_queue_peek_tail(q))])
  {
    g_queue_pop_tail(q);
  }

  //now we can increment
  ++frame->history_end;

  if (frame->history_end == frame->history_size)
  {
    frame->history_end = 0;
  }

  //then if the end has caught up to the start, we can increment the start
  if (frame->history_end == frame->history_start)
  {
    ++frame->history_start;

    //if the last value was the maximum, drop it
    if (!g_queue_is_empty(q) && 
      GPOINTER_TO_INT(g_queue_peek_head(q)) == frame->history_end)
    {
      g_queue_pop_head(q);
    }
  }

  g_queue_push_tail(q, GINT_TO_POINTER(frame->history_end));

  //the maximum is the head of the queue
  frame->history_max = GPOINTER_TO_INT(g_queue_peek_head(q));

  if (frame->history_start == frame->history_size)
  {
    frame->history_start = 0;
  }

#if 0
  //TODO min and max

  //at the moment, just count the max of the whole array
  i = 0;
  double max = 0;
  while (i != fields)
  {
    size_t j = 0;
    while (j != frame->history_size)
    {
      if (frame->history[i][j] > max)
      {
        max = frame->history[i][j];
      }
      ++j;
    }
    ++i;
  }

  frame->history_max = max;
#endif
}

static void
update_frame(FrameData* frame)
{
  SysinfoPluginData data;
  (*frame->plugin->get_data)(frame->plugin, &data);

  update_history(frame, frame->plugin->num_data, data.data);

  if (gtk_widget_get_visible(frame->tooltip_text))
  {
    update_tooltip(frame);
  }
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

  //do one update
  update(sysinfo);

  sysinfo->timeout_id = g_timeout_add(250, (GSourceFunc)update, sysinfo);

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

  /* get the orientation of the plugin */
  orientation = xfce_panel_plugin_get_orientation (plugin);

  gint h, w;

  /* set the widget size */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
  {
    h = size;
    w = DEFAULT_WIDTH;
  }
  else
  {
    w = size;
    h = DEFAULT_WIDTH;
  }
  
  size_t i = 0;
  while (i != sysinfo->num_displayed)
  {
    sysinfo->drawn_frames[i].width = w;
    //gtk_widget_set_size_request (GTK_WIDGET(plugin), w, h);
    //gtk_widget_set_size_request (sysinfo->drawn_frames[i].frame, w, h);
    gtk_widget_set_size_request (sysinfo->drawn_frames[i].drawing, w, h);
    ++i;
  }

  /* we handled the orientation */
  return TRUE;
}

static void
cleanup_frame(FrameData* f)
{
  gtk_widget_destroy(f->tooltip_text);
  g_queue_free(f->slidingqueue);
}

static void
sysinfo_free(SysinfoInstance* sysinfo)
{
  //free each plugin
  size_t i = 0;
  size_t num_plugins = sysinfo_pluginlist_size(sysinfo->plugin_list);

  while (i != num_plugins)
  {
    SysinfoPlugin* plugin = sysinfo_pluginlist_get(sysinfo->plugin_list, i);
    (*plugin->close)(plugin);
    ++i;
  }

  g_slice_free(SysinfoInstance, sysinfo);

  i = 0;
  while (i != sysinfo->num_displayed)
  {
    cleanup_frame(&sysinfo->drawn_frames[i]);
    ++i;
  }

  glibtop_close();
}

static void
sysinfo_init(XfcePanelPlugin* plugin)
{
  //create new plugin
  SysinfoInstance* sysinfo = sysinfo_construct(plugin);

  //connect some signals
  g_signal_connect (G_OBJECT(plugin), "free-data",
                    G_CALLBACK (sysinfo_free), sysinfo);
  g_signal_connect (G_OBJECT(plugin), "size-changed",
                    G_CALLBACK (size_changed_cb), sysinfo);
  g_signal_connect (G_OBJECT(plugin), "orientation-changed", 
                    G_CALLBACK(orientation_cb), sysinfo);

}

XFCE_PANEL_PLUGIN_REGISTER(sysinfo_init);
