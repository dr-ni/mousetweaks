AC_PREREQ(2.57)
AC_INIT([mousetweaks],
	[2.29.2],
	[http://bugzilla.gnome.org/enter_bug.cgi?product=mousetweaks])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([1.9])
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_INSTALL
AC_HEADER_STDC
AC_C_CONST
AM_PROG_CC_C_O

GNOME_COMMON_INIT
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

dnl *** i18n ***

IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=mousetweaks
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AM_GLIB_GNU_GETTEXT

dnl *** documentation ***

GNOME_DOC_INIT

dnl *** dependencies ***

PKG_CHECK_MODULES(DEPENDENCIES,
	gtk+-2.0 >= 2.16
	gconf-2.0 >= 2.16.0
	dbus-glib-1 >= 0.7
	cspi-1.0
	x11
	xcursor
	xfixes
	xtst)

AC_SUBST(DEPENDENCIES_CFLAGS)
AC_SUBST(DEPENDENCIES_LIBS)

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

dnl *** libpanelapplet ***

if test "$build_dca" = yes -o "$build_pca" = yes; then
	PKG_CHECK_MODULES(PANELAPPLET, libpanelapplet-2.0)
	AC_SUBST(PANELAPPLET_CFLAGS)
	AC_SUBST(PANELAPPLET_LIBS)
fi

dnl *** gconf ***

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
	AC_MSG_ERROR([gconftool-2 executable not found in your path])
fi

AM_GCONF_SOURCE_2

dnl *** output ***

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
