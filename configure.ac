#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xfce4-sysinfo-plugin], [0.1], [jarro.2783@gmail.com])
AC_CONFIG_SRCDIR([src/sysinfo.c])

AM_INIT_AUTOMAKE([dist-bzip2 dist-xz])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

# Check for libraries
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.10.0])
XDT_CHECK_PACKAGE([LIBGTOP], [libgtop-2.0], [2.28.5])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 plugins/Makefile
                 plugins/cpu/Makefile
                 include/Makefile
                 include/xfce4-sysinfo-plugin/Makefile
                ])
AC_OUTPUT
