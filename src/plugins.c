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

#include "plugins.h"

#define LIST_SIZE_INIT 5
#define LIST_SIZE_REALLOC 2

struct sysinfoplugin
{
};

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

SysinfoPluginList*
sysinfo_load_plugins(SysinfoInstance* sysinfo)
{
  SysinfoPluginList* list = sysinfo_pluginlist_new();
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
