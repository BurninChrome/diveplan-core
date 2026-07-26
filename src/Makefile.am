lib_LTLIBRARIES = libbuhlmann.la
libbuhlmann_la_SOURCES = buhlmann.c \
						 haldane.c \
						 schreiner.c \
						 zh-l12.c \
						 zh-l16.c \
						 compartment.c \
						 alveolar.c \
						 ceiling.c \
						 stop.c \
						 otu.c \
						 gradientfactor.c
# Applies to every target in this directory, including the `dive` demo, which
# previously compiled with no warning flags at all. Per-target _CFLAGS below
# override this rather than adding to it, so the library repeats what it needs.
AM_CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE

libbuhlmann_la_CFLAGS = -Wall -Wextra -O2 -std=c99 -D_GNU_SOURCE
include_HEADERS = buhlmann.h
noinst_PROGRAMS = dive
dive_LDFLAGS = -static
dive_LDADD = libbuhlmann.la
dive_SOURCES = dive.c getline.c
