AC_PREREQ(2.57)
AC_INIT([mousetweaks],
	[2.23.5],
	[http://bugzilla.gnome.org/enter_bug.cgi?product=mousetweaks], 
	[mousetweaks])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([1.9])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_INSTALL
AC_HEADER_STDC
AC_C_CONST

dnl *** i18n ***

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=mousetweaks
AC_SUBST(GETTEXT_PACKAGE)

AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/po/LINGUAS'])

dnl *** documentation ***

GNOME_DOC_INIT

dnl *** dependencies ***

PKG_CHECK_MODULES(DEPENDENCIES,
	gtk+-2.0 >= 2.10.0
	libglade-2.0 >= 2.4.0
	gconf-2.0 >= 2.16.0
	dbus-glib-1 >= 0.7
	libpanelapplet-2.0
	libgnome-2.0
	cspi-1.0
	xfixes)

AC_SUBST(DEPENDENCIES_CFLAGS)
AC_SUBST(DEPENDENCIES_LIBS)

dnl *** gconf ***

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
	AC_MSG_ERROR([gconftool-2 executable not found in your path])
fi

AM_GCONF_SOURCE_2

dnl *** arguments ***

AC_ARG_ENABLE(pointer-capture,
  [  --disable-pointer-capture     Disable pointer-capture applet],
  [build_pca=$enableval], [build_pca=yes])

AM_CONDITIONAL(BUILD_PCA, test "x$build_pca" = "xyes")
if test "$build_pca" = yes; then
  AC_DEFINE(BUILD_PCA, 1, [Build pointer-capture applet])
fi

AC_ARG_ENABLE(dwell-click,
  [  --disable-dwell-click         Disable dwell-click applet],
  [build_dca=$enableval], [build_dca=yes])

AM_CONDITIONAL(BUILD_DCA, test "x$build_dca" = "xyes")
if test "$build_dca" = yes; then
  AC_DEFINE(BUILD_DCA, 1, [Build dwell-click applet])
fi

AC_OUTPUT([Makefile
	   src/Makefile
	   data/Makefile
	   pixmaps/Makefile
	   po/Makefile.in
	   help/Makefile
	   man/Makefile])

echo ""
echo "Building panel applets:"
echo "-----------------------"
echo "  pointer-capture: $build_pca"
echo "  dwell-click:     $build_dca"
echo ""
