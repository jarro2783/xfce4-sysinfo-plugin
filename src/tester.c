#include "xfce4-sysinfo-plugin/xfce4_sysinfo_plugin.h"
#include "xfce4-sysinfo-plugin/plugins.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
  sysinfo_initialise();

  SysinfoPluginList* plugins = sysinfo_load_plugins();

  size_t size = sysinfo_pluginlist_size(plugins);

  printf("Found %zu plugins\n", size);

  if (size == 0)
  {
    printf("Quitting\n");
    exit(0);
  }

  size_t i = 0;
  while (i != size)
  {
    SysinfoPlugin* p = sysinfo_pluginlist_get(plugins, i);
    printf("%s\n", p->plugin_name);
    ++i;
  }

  printf("Printing data every 250 milliseconds, press ^C to quit\n");

  while (1)
  {
    usleep(250000);
    i = 0;
    while (i != size)
    {
      SysinfoPlugin* p = sysinfo_pluginlist_get(plugins, i);
      printf("%s\n", p->plugin_name);

      SysinfoPluginData data;

      (*p->get_data)(p, &data);

      //ignore background data
      size_t j = 1;
      while (j != p->num_data)
      {
        printf("%g\t", data.data[j]);
        ++j;
      }

      printf("\n");

      ++i;
    }
  }

  return 0;
}
