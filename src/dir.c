/* directory utilities

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

#include <glib/gstdio.h>

#define PLUGIN_NAMES_SIZE 10

gchar** 
sysinfo_dir_get_plugins(const char* path)
{
  size_t capacity = PLUGIN_NAMES_SIZE;
  size_t current = 0;

  //we keep one more for the null ptr at the end
  //this way it is always set, because the real capacity is one
  //more than the capacity that we use
  gchar** result = g_new0(gchar*, PLUGIN_NAMES_SIZE + 1);

  GDir* dir = g_dir_open(path, 0, 0);

  if (dir == 0)
  {
    return result;
  }

  const gchar* fname = g_dir_read_name(dir);

  while (fname != 0)
  {
    if (current >= capacity)
    {
      result = g_renew(gchar*, result, capacity * 2 + 1);
      
      //set to zeroes
      memset(result + capacity, 0, capacity / 2 + 1);
      capacity *= 2;
    }

    gchar* copy = g_strdup(fname);
    result[current] = copy;

    ++current;

    fname = g_dir_read_name(dir);
  }

  g_dir_close(dir);

  return result;
}
