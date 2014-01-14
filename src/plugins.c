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

#include <string.h>
#include <ltdl.h>
#include <glib/gprintf.h>

#include "xfce4-sysinfo-plugin/plugins.h"
#include "xfce4-sysinfo-plugin/dir.h"

#define LIST_SIZE_INIT 5
#define LIST_SIZE_REALLOC 2

struct sysinfopluginlist
{
  SysinfoPlugin** plugins;
  size_t capacity;
  size_t num_plugins;
};

SysinfoPluginList*
sysinfo_pluginlist_new()
{
  SysinfoPluginList* list = g_new(SysinfoPluginList, 1);
  list->plugins = g_new0(SysinfoPlugin*, LIST_SIZE_INIT);
  list->capacity = LIST_SIZE_INIT;
  list->num_plugins = 0;

  return list;
}

//The way this work is that we first get a list of all the files in the
//plugins directory. Then, if it looks like a library, we try to open it.
//If that works, we load it. The way we test if it is a library is that it
//cannot start with '.', and it must end in '.so'
SysinfoPluginList*
sysinfo_load_plugins()
{
  lt_dlsetsearchpath(SYSINFO_PLUGIN_DIR);

  SysinfoPluginList* list = sysinfo_pluginlist_new();

  gchar** files = sysinfo_dir_get_plugins(SYSINFO_PLUGIN_DIR);

  size_t current = 0;
  while (files[current] != 0)
  {
    size_t len = strlen(files[current]);

    if (strcmp(".la", files[current] + (len - 3)) == 0)
    {
      //it looks like a library so try to open it
      SysinfoPlugin* plugin = sysinfo_tryload_plugin(files[current]);

      if (plugin != 0)
      {
        sysinfo_pluginlist_append(list, plugin);
      }
    }

    g_free(files[current]);

    ++current;
  }

  return list;
}

SysinfoPlugin*
sysinfo_tryload_plugin(const char* file)
{
  //use lt_dlopen, which relies on the libtool search path to be set
  //the internal code does this, but if users want to write code
  //to open up individual plugins, they must must set the libtool search path

  lt_dlhandle dl = lt_dlopen(file);

  if (dl == 0)
  {
    return 0;
  }

  void* init = lt_dlsym(dl, "sysinfo_data_plugin_init");

  if (init == 0)
  {
    return 0;
  }

  return (*(sysinfo_plugin_init)init)();
}

size_t
sysinfo_pluginlist_size(SysinfoPluginList* list)
{
  return list->num_plugins;
}

SysinfoPlugin*
sysinfo_pluginlist_get(SysinfoPluginList* list, size_t i)
{
  if (list->num_plugins <= i)
  {
    g_error(
      "Requested plugin, %zu, "
      "is greater than the number of available plugins, %zu", 
      i, list->num_plugins);
  }

  return list->plugins[i];
}

void
sysinfo_pluginlist_append(SysinfoPluginList* list, SysinfoPlugin* plugin)
{
  if (list->capacity == list->num_plugins)
  {
    list->plugins = g_renew(SysinfoPlugin*, list->plugins, 
      list->capacity * LIST_SIZE_REALLOC);
    list->capacity *= LIST_SIZE_REALLOC;
  }

  list->plugins[list->num_plugins] = plugin;
  ++list->num_plugins;
}
