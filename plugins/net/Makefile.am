plugindir = $(libdir)/xfce4/panel/plugins/sysinfo

plugin_LTLIBRARIES = libsysinfo-net.la

libsysinfo_net_la_SOURCES = sysinfo_net.c

libsysinfo_net_la_CFLAGS = \
        $(LIBGTOP_CFLAGS) \
        -I$(top_srcdir)/include

libsysinfo_net_la_LIBADD = $(LIBGTOP_LIBS)

libsysinfo_net_la_LDFLAGS = \
        -avoid-version \
        -module \
        -no-undefined
