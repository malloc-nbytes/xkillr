AC_INIT([xkillr], [1.0], [zdhdev@yahoo.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])

# Check for ncurses
PKG_CHECK_MODULES([NCURSES], [ncurses], [], [AC_MSG_ERROR([ncurses library is required])])

# Set optimization flag
CFLAGS="$CFLAGS -O2"

AC_DEFINE_UNQUOTED([COMPILER_NAME], ["$CC"], [Name of the C compiler])
AC_DEFINE_UNQUOTED([COMPILER_VERSION], ["`$CC --version | head -n1`"], [Version of the C compiler])
AC_DEFINE_UNQUOTED([COMPILER_PATH], ["`which $CC`"], [Full path to the C compiler])
AC_DEFINE_UNQUOTED([COMPILER_FLAGS], ["$AM_CFLAGS"], [Compiler flags used])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
