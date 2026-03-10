AM_CFLAGS = -W -Wall -Wextra -std=c99 -D_GNU_SOURCE -I$(top_srcdir)/src -I$(top_builddir)
LDADD = $(top_builddir)/src/libbuhlmann.la -lm

check_PROGRAMS = test_buhlmann
test_buhlmann_SOURCES = test_buhlmann.c

TESTS = test_buhlmann
