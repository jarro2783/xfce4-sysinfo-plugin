/* plugins header

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
<http://www.gnu.org/licenses/>.  

*/

#ifndef SYSINFO_PLUGIN_PLUGINS_H
#define SYSINFO_PLUGIN_PLUGINS_H

#include <stddef.h>
#include <glib.h>

#define SYSINFO_PLUGIN_DIR PLUGIN_DIR "/sysinfo"

typedef struct 
{
  //an array of the data
  double* data;
} SysinfoPluginData;

typedef struct sysinfoplugin SysinfoPlugin;

typedef struct
{
  double red;
  double green;
  double blue;
} SysinfoColor;

typedef struct
{
  int which;
  SysinfoPlugin* plugin;
} SysinfoConfigColor;

struct sysinfoplugin
{
  gchar* plugin_name;
  int num_data;
  gchar** data_names;
  SysinfoColor* colors;

  SysinfoConfigColor* color_config;

  //data private to the plugin
  void* plugin_data;

  //gets the data for the current slice
  void (*get_data)(SysinfoPlugin*, SysinfoPluginData*);

  //get the range to display given the minimum and maximum values that have
  //been seen in the history
  void (*get_range)(SysinfoPlugin*, double min, double max, 
        double* display_min, double* display_max);

  //get the tooltip text
  gchar* (*get_tooltip)(SysinfoPlugin*);

  void (*reset_colors)(SysinfoPlugin*);

  //close the plugin
  void (*close)(SysinfoPlugin*);
};

typedef struct sysinfopluginlist SysinfoPluginList;

typedef SysinfoPlugin* (*sysinfo_plugin_init)();

SysinfoPluginList*
sysinfo_load_plugins();

SysinfoPlugin*
sysinfo_tryload_plugin(const char* file);

SysinfoPluginList*
sysinfo_pluginlist_new();

SysinfoPlugin*
sysinfo_pluginlist_remove(SysinfoPluginList* list, gchar* name);

SysinfoPlugin*
sysinfo_pluginlist_get_name(SysinfoPluginList* list, gchar* name);

size_t
sysinfo_pluginlist_size(SysinfoPluginList* list);

SysinfoPlugin*
sysinfo_pluginlist_get(SysinfoPluginList* list, size_t i);

void
sysinfo_pluginlist_append(SysinfoPluginList* list, SysinfoPlugin* plugin);

#endif
