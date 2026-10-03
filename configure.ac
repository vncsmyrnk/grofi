AC_INIT([grofi], [0.1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_PROG_CC
AM_PROG_AR
LT_INIT([disable-static])
PKG_CHECK_MODULES([ROFI], [rofi >= 2.0.0 glib-2.0 gmodule-2.0])

AC_PATH_PROG([NPM], [npm], [no])
AS_IF([test "x$NPM" = "xno"], [
  AC_MSG_ERROR([npm is required to compile TypeScript sources.])
])

PKG_CHECK_MODULES([GLIB], [glib-2.0 gio-2.0])
GLIB_GSETTINGS

AC_CONFIG_FILES([Makefile package.json])
AC_OUTPUT
