# GLOBAL CFLAGS
AM_CFLAGS   = -coverage -O2 -Wall 
AM_CPPFLAGS = -DLOCALEDIR='"$(localedir)"'
AM_CXXFLAGS = $(CFLAGS)

bin_PROGRAMS = pdir
pdir_SOURCES = src/main.c src/list.c
EXTRA_DIST = config.rpath  docs mano

ACLOCAL_AMFLAGS = -I ./m4

SUBDIRS = intl po
