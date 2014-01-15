plugindir = $(libdir)/xfce4/panel/plugins/sysinfo

plugin_LTLIBRARIES = libsysinfo-cpu.la

libsysinfo_cpu_la_SOURCES = sysinfo_cpu.c

libsysinfo_cpu_la_CFLAGS = \
        $(LIBGTOP_CFLAGS) \
        -I$(top_srcdir)/include

libsysinfo_cpu_la_LIBADD = $(LIBGTOP_LIBS)

libsysinfo_cpu_la_LDFLAGS = \
        -avoid-version \
        -module \
        -no-undefined
