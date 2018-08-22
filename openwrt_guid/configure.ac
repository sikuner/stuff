#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#AC_PREREQ([2.63])
AC_INIT([Beeba GUI], [1.0], [liguowang@beeba.cn])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AM_PROG_LIBTOOL
AC_PROG_CC

# Checks for libraries.
PKG_CHECK_MODULES(FREETYPE, freetype2)
AC_SUBST(FREETYPE_CFLAGS)
AC_SUBST(FREETYPE_LIBS)

#PKG_CHECK_MODULES(LIBICONV, libiconv)
#AC_SUBST(LIBICONV_CFLAGS)
#AC_SUBST(LIBICONV_LIBS)

AC_ARG_ENABLE(sdl, AS_HELP_STRING([--enable-sdl],[enable sdl support(default is no)]), enable_sdl=$enableval,enable_sdl=no)

if test x$enable_sdl = xyes; then
    AC_DEFINE(SDL_SUPPORT, 1, [enable sdl support])
    AM_CONDITIONAL(SDL_SUPPORT, true)  
fi

AM_CONDITIONAL(SDL_SUPPORT, [test "$enable_sdl" = "yes"])

AC_ARG_ENABLE(guid, AS_HELP_STRING([--enable-guid],[enable guid(default is no)]), enable_guid=$enableval,enable_guid=no)

if test x$enable_guid = xyes; then
    AC_DEFINE(GUID_SUPPORT, 1, [enable guid])
    AM_CONDITIONAL(GUID_SUPPORT, true)  
fi

AM_CONDITIONAL(GUID_SUPPORT, [test "$enable_guid" = "yes"])

AC_ARG_ENABLE(gui_cli, AS_HELP_STRING([--enable-gui_cli],[enable gui_cli support(default is no)]), enable_gui_cli=$enableval,enable_gui_cli=no)

if test x$enable_gui_cli = xyes; then
    AC_DEFINE(GUI_CLI_SUPPORT, 1, [enable gui_cli support])
    AM_CONDITIONAL(GUI_CLI_SUPPORT, true)  
fi

AM_CONDITIONAL(GUI_CLI_SUPPORT, [test "$enable_gui_cli" = "yes"])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_HEADER_MAJOR

AC_CONFIG_FILES([
     Makefile
     src/Makefile
     res/Makefile
     tools/Makefile
     tools/mp3_record/Makefile
     tools/sfvoice/Makefile
     ])
AC_OUTPUT
