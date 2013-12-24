/* cpu data for xfce4-sysinfo-plugin

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

#include <glibtop/cpu.h>

#include "xfce4-sysinfo-plugin/plugins.h"

void
sysinfo_data_plugin_init()
{
}

void
cpu_get_data()
{
  glibtop_cpu cpu;
  glibtop_get_cpu(&cpu);
}

static void
cpu_close()
{
}
