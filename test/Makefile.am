AM_CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE -I$(top_srcdir)/src -I$(top_builddir)
LDADD = $(top_builddir)/src/libbuhlmann.la -lm

check_PROGRAMS = test_buhlmann test_vectors test_profiles
test_buhlmann_SOURCES = test_buhlmann.c
test_vectors_SOURCES = test_vectors.c
test_profiles_SOURCES = test_profiles.c

# test_vectors and test_profiles read data from test/vectors/ and test/profiles/;
# TESTS_ENVIRONMENT exports srcdir so they find it in a VPATH build too.
TESTS = test_buhlmann test_vectors test_profiles
TESTS_ENVIRONMENT = srcdir='$(srcdir)'

# Test data ships with the source: a consumer reimplementing against this
# library needs the vectors, and a baseline is worthless if it is not versioned
# alongside the code it pins. Whole directories, not $(wildcard) — the latter
# expands against the build tree and so breaks a VPATH dist.
EXTRA_DIST = \
	vectors \
	profiles \
	xml \
	gen_dive.py \
	parse_dive.py \
	test_all_of_the_units.py
