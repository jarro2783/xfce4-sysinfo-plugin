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
#include <libxfce4ui/libxfce4ui.h>

#include <glibtop/close.h>

#define DEFAULT_HISTORY_SIZE 80
#define DEFAULT_WIDTH 60
#define DEFAULT_HEIGHT 40

#define COLOR_F_TO_16(c) (c * 65535)

//define this for extra debugging in tooltips
//#define TOOLTIP_DEBUG

typedef struct sysinfoinstance SysinfoInstance;
typedef struct framedata FrameData;
typedef struct pluginpagedata PluginPageData;

enum
{
  CONFIG_COL_ENABLED,
  CONFIG_COL_NAME,
  CONFIG_COL_FRAME_PTR,
  CONFIG_NUM_COLS
};

struct framedata
{
  SysinfoInstance* sysinfo;

  double** history;
  int history_start;
  int history_end;
  int history_size;
  double* history_sum;

  double history_max;
  int max_q_size;

  //which plugin are we handling
  SysinfoPlugin* plugin;

  #ifdef TOOLTIP_DEBUG
  gchar tt[300];
  #endif

  //our frame
  GtkWidget* frame;
  GtkWidget* drawing;
  GtkWidget* tooltip_text;

  //maximum of sliding window data
  GQueue* slidingqueue;

  gboolean shown;

  FrameData* nextframe;
};

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

struct pluginpagedata
{
  int num_colors;
  GtkColorButton** buttons;
  SysinfoPlugin* plugin;

  PluginPageData* next;
};

typedef struct
{
  SysinfoInstance* sysinfo;
  PluginPageData* page_data;
} ConfigDialogData;

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

  SysinfoColor* bg = &plugin->colors[0];

  gint width = w->allocation.width;
  gint height = w->allocation.height;

  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_source_rgb(cr, bg->red, bg->green, bg->blue);
  cairo_fill(cr);

  cairo_set_line_width(cr, 1);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);

  //if we have no data yet we can quit
  if (frame->history_sum == NULL)
  {
    return TRUE;
  }

  //compute where the zero is for this frame

  double min = 0;
  double max = 100;
  frame->plugin->get_range(plugin, 0, frame->history_max, &min, &max);

  double range = max - min;
  double scale = height / range;

  int base = height - scale * min;

  //draw each data point as a line from 0 to the value
  //the drawing area needs to be scaled appropriately

  int slices = frame->history_end - frame->history_start;
  slices = slices < 0 ? slices + frame->history_size : slices;
  int num_items = width < slices ? width : slices;

  int i = (frame->history_end - num_items);
  i = i < 0 ? i + frame->history_size : i;

  int nextbase = base;
  int x = width - num_items;
  //fprintf(stderr, "%s: drawing %d items from %d\n", frame->plugin->plugin_name,
  //  num_items, x);
  while (i != frame->history_end && i != frame->history_size)
  {
    nextbase = base;
    //start at 1 to ignore background
    size_t j = 1;
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
      //start plugin at 1 because we don't count background
      size_t j = 1;
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

  cairo_destroy(cr);

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
  #ifdef TOOLTIP_DEBUG
    #define LABEL_TEXT (fd->tt)
  #else
    #define LABEL_TEXT text
  #endif

  gchar* text = (*fd->plugin->get_tooltip)(fd->plugin);

  #ifdef TOOLTIP_DEBUG
  g_snprintf(
    fd->tt,
    300,
    "%s\nHistory Width = %d\nDrawing Width = %d",
    text,
    fd->history_size,
    fd->drawing->allocation.width
  );
  #endif

  gtk_label_set_text(GTK_LABEL(fd->tooltip_text), LABEL_TEXT);
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
resize_drawing(GtkWidget* w, GdkRectangle* alloc, FrameData* f)
{
  //reallocate history here now that we have the actual size of the drawing

  gint hw = alloc->width + 1;
  SysinfoPlugin* p = f->plugin;

  if (hw == f->history_size)
  {
    //it got resized to the same, so we have nothing to do
    return ;
  }

  //ignore background
  size_t j = 1;
  while (j != p->num_data)
  {
    if (f->history[j] == NULL)
    {
      f->history[j] = g_new0(double, hw);
    }
    else
    {
      f->history[j] = g_renew(double, f->history[j], hw);
    }
    f->history_size = hw;

    f->history_start = 0;
    f->history_end = 0;

    ++j;
  }

  if (f->history_sum == NULL)
  {
    f->history_sum = g_new0(double, hw);
  }
  else
  {
    f->history_sum = g_renew(double, f->history_sum, hw);
  }

  //reset the sliding max queue
  g_queue_clear(f->slidingqueue);
}

static void
setup_frame(GtkBox* box, FrameData* fd, SysinfoPlugin* plugin)
{
  GtkWidget* frame = gtk_frame_new(NULL);
  GtkWidget* drawing = gtk_drawing_area_new();

  //we own a reference to the frame
  g_object_ref(frame);

  gtk_container_add(GTK_CONTAINER(frame), drawing);
  //gtk_box_pack_start(box, frame, TRUE, TRUE, 0);

  fd->frame = frame;
  fd->drawing = drawing;

  g_signal_connect_after(G_OBJECT(drawing), "expose-event",
                   G_CALLBACK(draw_graph_cb), fd);

  //connect the resized event
  g_signal_connect_after(
    G_OBJECT(drawing), 
    "size-allocate",
    G_CALLBACK(resize_drawing), fd
  );

  fd->plugin = plugin;
  fd->history = g_new0(double*, plugin->num_data);

  //allocate a history for each component of the data
  size_t j = 1;
  while (j != plugin->num_data)
  {
    fd->slidingqueue = g_queue_new();
    ++j;
  }

  gtk_widget_set_has_tooltip(drawing, TRUE);
  fd->tooltip_text = gtk_label_new(NULL);
  g_object_ref(fd->tooltip_text);
  g_signal_connect(drawing, "query-tooltip", G_CALLBACK(tooltip_cb), fd);
 
  fd->shown = TRUE;
}

static void
construct_gui(XfcePanelPlugin* plugin, SysinfoInstance* sysinfo)
{
  GtkOrientation orientation = xfce_panel_plugin_get_orientation(plugin);

  sysinfo->top = gtk_event_box_new();
  //gtk_event_box_set_visible_window(GTK_EVENT_BOX(sysinfo->top), FALSE);
  gtk_event_box_set_above_child(GTK_EVENT_BOX(sysinfo->top), TRUE);

  xfce_panel_plugin_add_action_widget(plugin, sysinfo->top);

  gtk_container_add(GTK_CONTAINER(plugin), sysinfo->top);

  sysinfo->hvbox = xfce_hvbox_new(orientation, FALSE, 0);
  xfce_panel_plugin_add_action_widget(plugin, sysinfo->hvbox);
  gtk_container_add(GTK_CONTAINER(sysinfo->top), sysinfo->hvbox);

  //each graph is a drawing area in a frame
  size_t num_plugins = sysinfo_pluginlist_size(sysinfo->plugin_list);
  size_t i = 0;

  sysinfo->num_displayed = num_plugins;

  //fprintf(stderr, "opened %zu plugins\n", num_plugins);

  FrameData* head = 0;

  while (i != num_plugins)
  {
    FrameData* fd = g_slice_new0(FrameData);
    SysinfoPlugin* plugin = sysinfo_pluginlist_get(sysinfo->plugin_list, i);

    fd->sysinfo = sysinfo;

    setup_frame(GTK_BOX(sysinfo->hvbox), fd, plugin);

    fd->nextframe = head;
    head = fd;

    ++i;
  }

  sysinfo->drawn_frames = head;

  //go through in forward order and pack frames
  FrameData* current = sysinfo->drawn_frames;
  while (current)
  {
    gtk_box_pack_start(GTK_BOX(sysinfo->hvbox), current->frame, TRUE, TRUE, 0);
    current = current->nextframe;
  }

  gtk_widget_show_all(sysinfo->top);
}

static void
update_history(FrameData* frame, int fields, double* data)
{
  //add a new data point into the history
  //computes the new min and max

  //if we have no history, we can quit
  if (frame->history_sum == NULL)
  {
    return;
  }

  //fill the data first, start at 1 to ignore background
  size_t i = 1;

  int sum = 0;
  while (i != fields)
  {
    frame->history[i][frame->history_end] = data[i];
    sum += data[i];
    ++i;
  }

  frame->history_sum[frame->history_end] = sum;

  int oldend = frame->history_end;

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

  g_queue_push_tail(q, GINT_TO_POINTER(oldend));

  //the maximum is the head of the queue
  frame->history_max = 
    frame->history_sum[GPOINTER_TO_INT(g_queue_peek_head(q))];

  if (frame->history_start == frame->history_size)
  {
    frame->history_start = 0;
  }

  frame->max_q_size = g_queue_get_length(q);
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
  FrameData* frame = sysinfo->drawn_frames;
  while (frame != 0)
  {
    if (frame->shown)
    {
      update_frame(frame);
      gtk_widget_queue_draw(frame->drawing);
    }
    frame = frame->nextframe;
  }

  return TRUE;
}

static FrameData*
frame_remove_plugin(SysinfoInstance* sysinfo, gchar* name)
{
  //pointer to the previous pointer
  FrameData** previous = &sysinfo->drawn_frames;

  //the current frame
  FrameData* current = sysinfo->drawn_frames;
  FrameData* result = 0;

  while (current != 0 && result == 0)
  {
    if (strcmp(name, current->plugin->plugin_name) == 0)
    {
      result = current;
      //take out the current frame
      *previous = current->nextframe;
      current->nextframe = 0;
    }
    else
    {
      previous = &current->nextframe;
      current = current->nextframe;
    }
  }

  return result;
}

static double
read_one_color(gchar* hex)
{
  int result;
  sscanf(hex, "%x", &result);
  return result / 255.;
}

static void
plugin_set_color(gchar** color, SysinfoPlugin* plugin, int which)
{
  plugin->colors[which].red = read_one_color(color[0]);
  plugin->colors[which].green = read_one_color(color[1]);
  plugin->colors[which].blue = read_one_color(color[2]);
}

static void
read_config(SysinfoInstance* sysinfo)
{
  gchar* save_location = xfce_panel_plugin_lookup_rc_file(sysinfo->plugin);

  fprintf(stderr, "restoring from %s\n", save_location);

  if (save_location == 0)
  {
    return ;
  }

  //restore the config from the file
  XfceRc* rc = xfce_rc_simple_open(save_location, TRUE);

  if (rc == 0)
  {
    //there was an error so we don't restore
    return ;
  }

  xfce_rc_set_group(rc, "sysinfo");

  int entries = xfce_rc_read_int_entry(rc, "num_plugins", 0);

  gchar** groups = xfce_rc_get_groups(rc);

  //reorder the plugins by putting them in an array and then
  //linking them up
  //if there are extras then they will be put at the end
  FrameData** frames = g_new0(FrameData*, entries + 1);

  int i = 0;
  while (groups[i] != 0)
  {
    xfce_rc_set_group(rc, groups[i]);

    int pos = xfce_rc_read_int_entry(rc, "position", 0);

    FrameData* f = frame_remove_plugin(sysinfo, groups[i]);

    //if it was found, put it in its place
    if (f != 0)
    {
      if (pos < entries)
      {
        //read all the colors
        int c = 0;
        while (c != f->plugin->num_data)
        {
          gchar** color = xfce_rc_read_list_entry(rc, 
            f->plugin->data_names[c], ",");

          plugin_set_color(color, f->plugin, c);

          g_strfreev(color);
          ++c;
        }

        frames[pos] = f;
      }
    }

    ++i;
  }

  //then go through the groups and reset the list
  i = 0;
  int position = 0;
  while (frames[i] == 0 && i != entries)
  {
    //find the first non empty frame
    ++i;
  }

  FrameData* head = frames[i];

  while (i != entries)
  {
    FrameData* f = frames[i];

    //put the frame in its position in the hbox
    gtk_box_reorder_child(GTK_BOX(sysinfo->hvbox), f->frame, position);

    //find the next one
    ++i;
    while (frames[i] == 0 && i != entries)
    {
      ++i;
    }

    ++position;
    f->nextframe = frames[i];
  }

  sysinfo->drawn_frames = head;

  g_strfreev(groups);

  xfce_rc_close(rc);
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

  read_config(sysinfo);

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
  //fprintf(stderr, "size changed to %d\n", size);
  GtkOrientation orientation;

  orientation = xfce_panel_plugin_get_orientation (plugin);

  gint h, w;

  if (orientation == GTK_ORIENTATION_HORIZONTAL)
  {
    h = size;
    w = DEFAULT_WIDTH;
  }
  else
  {
    w = size;
    h = DEFAULT_HEIGHT;
  }

  FrameData* frame = sysinfo->drawn_frames;
  while (frame != 0)
  {
    //gtk_widget_set_size_request (GTK_WIDGET(plugin), w, h);
    gtk_widget_set_size_request (frame->frame, w, h);
    //gtk_widget_set_size_request (frame->drawing, w, h);

    frame = frame->nextframe;
  }


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

  FrameData* fd = sysinfo->drawn_frames;
  FrameData* old;
  while (fd != 0)
  {
    cleanup_frame(fd);

    old = fd;
    fd = fd->nextframe;

    g_slice_free(FrameData, old);
  }

  g_slice_free(SysinfoInstance, sysinfo);

  glibtop_close();
}

static void
format_one_color_string(gchar* dest, double color)
{
  fprintf(stderr, "Printing colour %g as %.2x\n", color, (int)(color*255));
  snprintf(dest, 3, "%.2x", (int)(color * 255));
}

static gchar** 
format_color_string(SysinfoColor* c)
{
  //write the color as a 2 digit hex
  gchar** colors = g_new(gchar*, 4);
  colors[3] = 0;

  int i = 0;
  while (i != 3)
  {
    colors[i] = g_new(gchar, 3);
    ++i;
  }

  format_one_color_string(colors[0], c->red);
  format_one_color_string(colors[1], c->green);
  format_one_color_string(colors[2], c->blue);

  return colors;
}

static void
free_color_string(gchar** colors)
{
  int i = 0;
  while (i != 3)
  {
    g_free(colors[i]);
    ++i;
  }

  g_free(colors);
}

static void
save_plugin(XfcePanelPlugin* plugin, SysinfoInstance* sysinfo)
{
  gchar* save_file = xfce_panel_plugin_save_location(plugin, TRUE);

  XfceRc* rc = xfce_rc_simple_open(save_file, FALSE);

  if (rc == 0)
  {
    return ;
  }

  int pos = 0;
  FrameData* frame = sysinfo->drawn_frames;

  while (frame != 0)
  {
    xfce_rc_set_group(rc, frame->plugin->plugin_name);

    xfce_rc_write_int_entry(rc, "position", pos);

    int which_data = 0;
    while (which_data != frame->plugin->num_data)
    {
      fprintf(stderr, "formitting colours for %s: %s\n", 
        frame->plugin->plugin_name, frame->plugin->data_names[which_data]);
      gchar** colors = format_color_string(&frame->plugin->colors[which_data]);
      xfce_rc_write_list_entry(rc, frame->plugin->data_names[which_data],
        colors, ",");

      free_color_string(colors);

      ++which_data;
    }

    ++pos;
    frame = frame->nextframe;
  }

  //pos will now be the number of plugins
  xfce_rc_set_group(rc, "sysinfo");
  xfce_rc_write_int_entry(rc, "num_plugins", pos);

  xfce_rc_close(rc);
}

static void
config_response_cb(GtkWidget* dlg, gint response, ConfigDialogData* data)
{
  //free the memory for all the plugin configs
  PluginPageData* current = data->page_data;
  PluginPageData* next = 0;

  while (current != 0)
  {
    g_free(current->buttons);

    next = current->next;
    g_free(current);
    current = next;
  }

  gtk_widget_destroy(dlg);

  //save configuration to file
  save_plugin(data->sysinfo->plugin, data->sysinfo);

  xfce_panel_plugin_unblock_menu(data->sysinfo->plugin);

  g_free(data);
}

static void
config_toggle_enabled
(
  GtkCellRendererToggle* renderer,
  gchar* path,
  GtkListStore* model
)
{
  GtkTreeIter iter;
  gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(model), &iter, path);

  gboolean enabled;
  gtk_tree_model_get(
    GTK_TREE_MODEL(model), 
    &iter, 
    CONFIG_COL_ENABLED,
    &enabled,
    -1
  );

  enabled = !enabled;

  gtk_list_store_set(model, &iter,
    CONFIG_COL_ENABLED, enabled,
    -1);

  FrameData* fd;

  gtk_tree_model_get(
    GTK_TREE_MODEL(model),
    &iter,
    CONFIG_COL_FRAME_PTR,
    &fd,
    -1
  );

  fd->shown = enabled;

  gtk_widget_set_visible(fd->frame, enabled);
}

static void
drag_data_inserted_cb
(
  GtkTreeModel* model,
  GtkTreePath* path,
  GtkTreeIter* iter,
  SysinfoInstance* sysinfo
)
{
}

static void
drag_data_changed_cb
(
  GtkTreeModel* model,
  GtkTreePath* path,
  GtkTreeIter* iter,
  SysinfoInstance* sysinfo
)
{
}

static void
drag_data_deleted_cb
(
  GtkTreeModel* model,
  GtkTreePath* path,
  SysinfoInstance* sysinfo
)
{
  FrameData* new_head = 0;
  //this is the last function called, so we can work out the new order

  //first build the list in the reverse order
  GtkTreeIter iter;
  gboolean has_next = gtk_tree_model_get_iter_first(model, &iter);

  int num = 0;

  while (has_next)
  {
    gchar* name;
    gtk_tree_model_get(model, &iter, 1, &name, -1);

    FrameData* frame = frame_remove_plugin(sysinfo, name);
    frame->nextframe = new_head;
    new_head = frame;

    g_free(name);

    has_next = gtk_tree_model_iter_next(model, &iter);

    ++num;
  }

  //then reverse the list and reorder the widgets
  FrameData* current = new_head;
  FrameData* next = 0;
  new_head = 0;

  while (current)
  {
    next = current->nextframe;
    current->nextframe = new_head;
    new_head = current;
    current = next;
  }
  
  sysinfo->drawn_frames = new_head;

  //go through in forward order and reorder the frames
  int i = 0;
  current = sysinfo->drawn_frames;
  while (current)
  {
    gtk_box_reorder_child(GTK_BOX(sysinfo->hvbox), 
      GTK_WIDGET(current->frame), i);

    current = current->nextframe;
    ++i;
  }
}

static void
make_sys_configuration(GtkBox* c, SysinfoInstance* sysinfo)
{
  GtkWidget* update_row = gtk_hbox_new(FALSE, 0);

  gtk_box_pack_start(c, update_row, FALSE, FALSE, 0);

  GtkWidget* update_text = gtk_label_new("Update interval:");

  gtk_box_pack_start(GTK_BOX(update_row), update_text, FALSE, FALSE, 10);

  GtkWidget* update_intervals = gtk_combo_box_text_new();

  gchar* intervals[] = 
    {
      "250ms",
      "500ms",
      "750ms",
      "1000ms",
      NULL
    };

  int i = 0;
  while (intervals[i])
  {
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(update_intervals), 
      intervals[i]);
    ++i;
  }

  gtk_box_pack_start(GTK_BOX(update_row), update_intervals, FALSE, FALSE, 0);
  gtk_combo_box_set_active(GTK_COMBO_BOX(update_intervals), 0);

  //make the tree view
  //loop through every plugin and make an entry for it
  FrameData* frame = sysinfo->drawn_frames;

  GtkListStore* store = gtk_list_store_new(
    CONFIG_NUM_COLS, 
    G_TYPE_BOOLEAN, 
    G_TYPE_STRING,
    G_TYPE_POINTER
  );

  GtkTreeIter iter;
  
  while (frame != 0)
  {
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(
      store, &iter,
      CONFIG_COL_ENABLED, frame->shown,
      CONFIG_COL_NAME, frame->plugin->plugin_name,
      CONFIG_COL_FRAME_PTR, frame,
      -1
    );

    frame = frame->nextframe;
  }

  GtkWidget* tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

  gtk_tree_view_set_reorderable(GTK_TREE_VIEW(tree), TRUE);

  gtk_box_pack_start(c, tree, FALSE, FALSE, 10);

  GtkCellRenderer* renderer;
  GtkTreeViewColumn* column;

  renderer = gtk_cell_renderer_toggle_new();
  column = gtk_tree_view_column_new_with_attributes(
    "Enabled", renderer,
    "active", CONFIG_COL_ENABLED,
    NULL);

  g_signal_connect(renderer, "toggled", G_CALLBACK(&config_toggle_enabled), 
    store);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
  
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(
    "Name", renderer,
    "text", CONFIG_COL_NAME, 
    NULL);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  //connect the row change signals
  g_signal_connect(store, "row-inserted", 
    G_CALLBACK(&drag_data_inserted_cb), sysinfo);
  g_signal_connect(store, "row-changed", 
    G_CALLBACK(&drag_data_changed_cb), sysinfo);
  g_signal_connect(store, "row-deleted", 
    G_CALLBACK(&drag_data_deleted_cb), sysinfo);
}

static void
color_changed_cb(GtkColorButton* widget, SysinfoConfigColor* color)
{
  SysinfoColor* c = &color->plugin->colors[color->which];

  GdkColor result;
  gtk_color_button_get_color(widget, &result);

  c->red = result.red / 65535.;
  c->green = result.green / 65535.;
  c->blue = result.blue / 65535.;
}

static void
set_color_button(GtkColorButton* button, SysinfoPlugin* plugin, int which)
{
  GdkColor color = {
    0,
    COLOR_F_TO_16(plugin->colors[which].red), 
    COLOR_F_TO_16(plugin->colors[which].green),
    COLOR_F_TO_16(plugin->colors[which].blue)
    };

  gtk_color_button_set_color(button, &color);
}

static void
reset_button_cb(GtkButton* widget, PluginPageData* page)
{
  page->plugin->reset_colors(page->plugin);

  // now reset the actual color buttons
  int i = 0;
  while (i != page->num_colors)
  {
    set_color_button(page->buttons[i], page->plugin, i);
    ++i;
  }
}

static void
add_plugin_pages(GtkNotebook* book, ConfigDialogData* data)
{

  FrameData* frame = data->sysinfo->drawn_frames;

  while (frame != 0)
  {
    //make a horizontal list of colour names and a colour picker
    //with a reset button underneath
    GtkWidget* page;
    GtkWidget* page_vbox;
    GtkWidget* page_label;
    GtkWidget* color_table;
    GtkWidget* table_hbox;

    GtkWidget* reset_button;
    GtkWidget* reset_hbox;

    GtkWidget* color_label = 0;
    GtkWidget* color_button = 0;

    SysinfoPlugin* plugin = frame->plugin;

    page_vbox = gtk_vbox_new(FALSE, 0);

    color_table = gtk_table_new(2, plugin->num_data, FALSE);
    table_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(table_hbox), color_table, TRUE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page_vbox), table_hbox, FALSE, FALSE, 0);

    reset_button = gtk_button_new_with_label("Reset");
    reset_hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(reset_hbox), reset_button, TRUE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page_vbox), reset_hbox, FALSE, FALSE, 10);

    page = page_vbox;

    page_label = gtk_label_new(frame->plugin->plugin_name);
    gtk_notebook_append_page(book, page, page_label);

    //fill each page with the data for that plugin
    //so far we only do colour

    PluginPageData* page_data = g_new0(PluginPageData, 1);

    page_data->num_colors = plugin->num_data;
    page_data->buttons = g_new(GtkColorButton*, page_data->num_colors);
    page_data->plugin = plugin;

    int i = 0;
    while (i != plugin->num_data)
    {

      color_label = gtk_label_new(plugin->data_names[i]);

      gtk_table_attach(GTK_TABLE(color_table), color_label, 
        i, i + 1, 0, 1,
        0, 0, 6, 3);

      color_button = gtk_color_button_new();
      set_color_button(GTK_COLOR_BUTTON(color_button), plugin, i);

      g_signal_connect (G_OBJECT(color_button), "color-set",
        G_CALLBACK (color_changed_cb), &plugin->color_config[i]);

      gtk_table_attach(GTK_TABLE(color_table), color_button, 
        i, i + 1, 1, 2,
        0, 0, 6, 3);

      page_data->buttons[i] = GTK_COLOR_BUTTON(color_button);

      color_label = 0;
      ++i;
    }

    page_data->next = data->page_data;
    data->page_data = page_data;

    //connect the push signal for the reset button
    g_signal_connect (G_OBJECT(reset_button), "clicked",
      G_CALLBACK (reset_button_cb), page_data);

    frame = frame->nextframe;
  }
}

static void
configure_plugin(XfcePanelPlugin* plugin, SysinfoInstance* sysinfo)
{
  GtkWidget* dlg;
  GtkWidget* book;
  GtkWidget* front_child;
  GtkWidget* front_label;
  GtkWidget* content_area;
  ConfigDialogData* config_data;

  xfce_panel_plugin_block_menu(plugin);

  dlg = xfce_titled_dialog_new_with_buttons("System Info Properties",
    GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
    GTK_STOCK_CLOSE,
    GTK_RESPONSE_OK,
    NULL
  );

  config_data = g_new0(ConfigDialogData, 1);
  config_data->sysinfo = sysinfo;

  g_signal_connect(dlg, "response", G_CALLBACK(config_response_cb), 
    config_data);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

  front_child = gtk_vbox_new(FALSE, 0);
  book = gtk_notebook_new();
  front_label = gtk_label_new("Sysinfo");

  gtk_notebook_append_page(GTK_NOTEBOOK(book), front_child, front_label);

  gtk_container_add(GTK_CONTAINER(content_area), book); 

  make_sys_configuration(GTK_BOX(front_child), sysinfo);

  add_plugin_pages(GTK_NOTEBOOK(book), config_data);

  gtk_widget_show_all(dlg);
}

static void
sysinfo_init(XfcePanelPlugin* plugin)
{
  //create new plugin
  SysinfoInstance* sysinfo = sysinfo_construct(plugin);

  xfce_panel_plugin_menu_show_configure(plugin);

  //connect some signals
  g_signal_connect (G_OBJECT(plugin), "free-data",
    G_CALLBACK (sysinfo_free), sysinfo);
  g_signal_connect (G_OBJECT(plugin), "size-changed",
    G_CALLBACK (size_changed_cb), sysinfo);
  g_signal_connect (G_OBJECT(plugin), "orientation-changed", 
    G_CALLBACK(orientation_cb), sysinfo);
  g_signal_connect (plugin, "configure-plugin", 
    G_CALLBACK(configure_plugin), sysinfo);
  g_signal_connect (plugin, "save", 
    G_CALLBACK(save_plugin), sysinfo);

  //g_log_set_always_fatal(G_LOG_LEVEL_WARNING);

}

XFCE_PANEL_PLUGIN_REGISTER(sysinfo_init);
