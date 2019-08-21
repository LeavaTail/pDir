# GLOBAL CFLAGS
AM_CFLAGS   = -coverage -O2 -Wall 
AM_CPPFLAGS = -DLOCALEDIR='"$(localedir)"' -DPDIR_DEBUG
AM_CXXFLAGS = $(CFLAGS)

bin_PROGRAMS = pdir
pdir_SOURCES = src/main.c src/error.c src/list.c

man_MANS = man/pdir.1
EXTRA_DIST = config.rpath  docs mano

ACLOCAL_AMFLAGS = -I ./m4

SUBDIRS = intl po

# test script
TESTS = tests/init.sh
