plugindir = $(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = libsysinfo.la

noinst_PROGRAMS = tester

tester_SOURCES = tester.c

tester_CFLAGS = \
        -I$(top_srcdir)/include \
				$(LIBGTOP_CFLAGS)

tester_LDADD = libsysinfo.la

libsysinfo_la_CFLAGS = \
        $(LIBXFCE4PANEL_CFLAGS) \
				$(LIBXFCE4UI_CFLAGS) \
        $(LIBGTOP_CFLAGS) \
        -DPLUGIN_DIR=\"$(plugindir)\" \
        -I$(top_srcdir)/include

libsysinfo_la_LIBADD = \
				$(LIBXFCE4PANEL_LIBS) \
				$(LIBXFCE4UI_LIBS) \
				$(LIBGTOP_LIBS) \
				$(LIBLTDL)

libsysinfo_la_LDFLAGS = \
        -avoid-version \
        -module \
        -no-undefined

#        -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' 

libsysinfo_la_SOURCES = \
        sysinfo.c \
        libsysinfo.c \
        plugins.c \
        dir.c

# desktop file
desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = sysinfo.desktop
