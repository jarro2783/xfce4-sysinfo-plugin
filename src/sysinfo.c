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

//define this for extra debugging in tooltips
//#define TOOLTIP_DEBUG

typedef struct sysinfoinstance SysinfoInstance;
typedef struct framedata FrameData;

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

  gint width = w->allocation.width;
  gint height = w->allocation.height;

  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_source_rgb(cr, 1, 1, 1);
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
  fprintf(stderr, "%s: drawing %d items from %d\n", frame->plugin->plugin_name,
    num_items, x);
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

  size_t j = 0;
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
  //GtkWidget* drawing = gtk_label_new("Hello world");

  gtk_container_add(GTK_CONTAINER(frame), drawing);
  gtk_box_pack_end(box, frame, TRUE, TRUE, 0);

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
  size_t j = 0;
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
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(sysinfo->top), FALSE);
  gtk_event_box_set_above_child(GTK_EVENT_BOX(sysinfo->top), TRUE);

  //xfce_panel_plugin_add_action_widget(plugin, sysinfo->top);

  gtk_container_add(GTK_CONTAINER(plugin), sysinfo->top);

  sysinfo->hvbox = xfce_hvbox_new(orientation, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(sysinfo->top), sysinfo->hvbox);

  //each graph is a drawing area in a frame
  size_t num_plugins = sysinfo_pluginlist_size(sysinfo->plugin_list);
  size_t i = 0;

  sysinfo->num_displayed = num_plugins;

  fprintf(stderr, "opened %zu plugins\n", num_plugins);

  FrameData* head = 0;

  while (i != num_plugins)
  {
    FrameData* fd = g_slice_new0(FrameData);
    SysinfoPlugin* plugin = sysinfo_pluginlist_get(sysinfo->plugin_list, i);

    setup_frame(GTK_BOX(sysinfo->hvbox), fd, plugin);

    fd->nextframe = head;
    head = fd;

    ++i;
  }

  sysinfo->drawn_frames = head;

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

  //fill the data first
  size_t i = 0;

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
  fprintf(stderr, "size changed to %d\n", size);
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
config_response_cb(GtkWidget* dlg, gint response, SysinfoInstance* sysinfo)
{
  gtk_widget_destroy(dlg);
  xfce_panel_plugin_unblock_menu(sysinfo->plugin);
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
      CONFIG_COL_ENABLED, TRUE,
      CONFIG_COL_NAME, frame->plugin->plugin_name,
      CONFIG_COL_FRAME_PTR, frame,
      -1
    );

    frame = frame->nextframe;
  }

  GtkWidget* tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

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
}

static void
configure_plugin(XfcePanelPlugin* plugin, SysinfoInstance* sysinfo)
{
  GtkWidget* dlg;
  GtkWidget* book;
  GtkWidget* front_child;
  GtkWidget* front_label;
  GtkWidget* content_area;

  xfce_panel_plugin_block_menu(plugin);

  dlg = xfce_titled_dialog_new_with_buttons("System Info Properties",
    GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
    GTK_STOCK_CLOSE,
    GTK_RESPONSE_OK,
    NULL
  );

  g_signal_connect(dlg, "response", G_CALLBACK(config_response_cb), sysinfo);

  content_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

  front_child = gtk_vbox_new(FALSE, 0);
  book = gtk_notebook_new();
  front_label = gtk_label_new("Sysinfo");

  gtk_notebook_append_page(GTK_NOTEBOOK(book), front_child, front_label);

  gtk_container_add(GTK_CONTAINER(content_area), book); 

  make_sys_configuration(GTK_BOX(front_child), sysinfo);

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

}

XFCE_PANEL_PLUGIN_REGISTER(sysinfo_init);
