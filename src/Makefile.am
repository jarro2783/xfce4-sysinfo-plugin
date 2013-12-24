plugindir = $(libexecdir)/xfce4/panel-plugins

plugin_LTLIBRARIES = libxfce4-sysinfo-plugin.la

libxfce4_sysinfo_plugin_la_CFLAGS = \
        $(LIBXFCE4PANEL_CFLAGS) \
        $(LIBGTOP_CFLAGS)

libxfce4_sysinfo_plugin_la_LIBADD = $(LIBXFCE4PANEL_LIBS)

libxfce4_sysinfo_plugin_la_LDFLAGS = \
        -avoid-version \
        -module \
        -no-undefined \
        -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' 

libxfce4_sysinfo_plugin_la_SOURCES = \
        xfce4_sysinfo_plugin.c \
        plugins.c \
        dir.c
