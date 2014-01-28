plugindir = $(libdir)/xfce4/panel/plugins/sysinfo

plugin_LTLIBRARIES = libsysinfo-mem.la

libsysinfo_mem_la_SOURCES = sysinfo_mem.c

libsysinfo_mem_la_CFLAGS = \
        $(LIBGTOP_CFLAGS) \
        -I$(top_srcdir)/include

libsysinfo_mem_la_LIBADD = $(LIBGTOP_LIBS)

libsysinfo_mem_la_LDFLAGS = \
        -avoid-version \
        -module \
        -no-undefined
