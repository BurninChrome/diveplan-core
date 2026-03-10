AC_INIT([libbuhlmann], [1.0], [contact@aquabsd.org])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE
LT_INIT
AC_CONFIG_SRCDIR([src/buhlmann.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [exp])

# Checks for header files.
AC_CHECK_HEADERS([math.h])

# Checks for library functions.
AC_CHECK_FUNCS([getline])

AC_CONFIG_FILES([Makefile src/Makefile test/Makefile])
AC_OUTPUT
