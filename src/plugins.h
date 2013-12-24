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

#include "xfce4_sysinfo_plugin.h"

#define SYSINFO_PLUGIN_DIR PLUGIN_DIR "/sysinfo"

typedef struct sysinfoplugin SysinfoPlugin;
typedef struct sysinfopluginlist SysinfoPluginList;

SysinfoPluginList*
sysinfo_load_plugins(SysinfoInstance* sysinfo);

SysinfoPlugin*
sysinfo_tryload_plugin(const char* file);

SysinfoPluginList*
sysinfo_pluginlist_new();

SysinfoPlugin*
sysinfo_pluginlist_get(SysinfoPluginList* list, size_t i);

void
sysinfo_pluginlist_append(SysinfoPluginList* list, SysinfoPlugin* plugin);

#endif
