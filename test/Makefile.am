AM_CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE -I$(top_srcdir)/src -I$(top_builddir)
LDADD = $(top_builddir)/src/libbuhlmann.la -lm

check_PROGRAMS = test_buhlmann test_vectors
test_buhlmann_SOURCES = test_buhlmann.c
test_vectors_SOURCES = test_vectors.c

# test_vectors reads test/vectors/*.tsv; TESTS_ENVIRONMENT exports srcdir so it
# finds them in a VPATH build too.
TESTS = test_buhlmann test_vectors
TESTS_ENVIRONMENT = srcdir='$(srcdir)'

# Vectors, their generator and the regression baseline ship with the source:
# a consumer reimplementing against this library needs the vectors, and the
# baseline is worthless if it is not versioned alongside the code it pins.
EXTRA_DIST = \
	vectors/gen_vectors.py \
	vectors/expected-failures.txt \
	vectors/README.md \
	vectors/alveolar.tsv \
	vectors/haldane.tsv \
	vectors/schreiner.tsv \
	vectors/ceiling.tsv \
	vectors/gradient-tolerance.tsv \
	vectors/ndl.tsv \
	vectors/zh-l16-derivation.tsv \
	baseline.sh \
	baseline.sha256 \
	gen_dive.py \
	parse_dive.py \
	test_all_of_the_units.py
