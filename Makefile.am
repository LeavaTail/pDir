# GLOBAL CFLAGS
AM_CFLAGS   = 
AM_CXXFLAGS = 

bin_PROGRAMS = pdir
pdir_SOURCES = src/main.c src/error.c src/list.c

pdir_CFLAGS = -DLOCALEDIR='"$(localedir)"'
if DEBUG
pdir_CFLAGS += -DPDIR_DEBUG -O0 -g3 -coverage -Wall
else
pdir_CFLAGS += -O2
endif

man_MANS = man/pdir.1
EXTRA_DIST = config.rpath  docs mano

ACLOCAL_AMFLAGS = -I ./m4

SUBDIRS = intl po

# test script
TESTS = tests/init.sh
