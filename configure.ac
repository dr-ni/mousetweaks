AC_PREREQ(2.57)

AC_INIT([mousetweaks], [2.21.5], [lowfi@chello.at], [mousetweaks])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([1.9])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_INSTALL
AC_HEADER_STDC
AC_C_CONST

dnl *************
dnl          i18n
dnl *************

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=mousetweaks
AC_SUBST(GETTEXT_PACKAGE)

AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AC_SUBST([CONFIG_STATUS_DEPENDENCIES],['$(top_srcdir)/po/LINGUAS'])

localedir=$datadir/locale
AC_SUBST(localedir)

GNOME_DOC_INIT

dnl *************
dnl          deps
dnl *************

PKG_CHECK_MODULES(GCONF, gconf-2.0, have_gconf=true, have_gconf=false)
if test "x${have_gconf}" = "xfalse"; then
        AC_MSG_ERROR([No gconf-2.0 package information found])
fi

AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
	AC_MSG_ERROR([gconftool-2 executable not found in your path])
fi

AM_GCONF_SOURCE_2

PKG_CHECK_MODULES(GTK, gtk+-2.0, have_gtk=true, have_gtk=false)
if test "x${have_gtk}" = "xfalse"; then
        AC_MSG_ERROR([No gtk+-2.0 package information found])
fi

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GLADE, libglade-2.0, have_glade=true, have_glade=false)
if test "x${have_glade}" = "xfalse"; then
	AC_MSG_ERROR([No libglade-2.0 package information found])
fi

AC_SUBST(GLADE_CFLAGS)
AC_SUBST(GLADE_LIBS)

AC_ARG_ENABLE(pointer-capture,
  [  --disable-pointer-capture     Disable pointer-capture applet],
  [build_pca=$enableval], [build_pca=yes])

AM_CONDITIONAL(BUILD_PCA, test "x$build_pca" = "xyes")
if test "$build_pca" = yes; then
  AC_DEFINE(BUILD_PCA, 1, [Build pointer-capture applet])
fi

AC_ARG_ENABLE(click-type,
  [  --disable-click-type          Disable click-type applet],
  [build_cta=$enableval], [build_cta=yes])

AM_CONDITIONAL(BUILD_CTA, test "x$build_cta" = "xyes")
if test "$build_cta" = yes; then
  AC_DEFINE(BUILD_CTA, 1, [Build click-type applet])
fi

PKG_CHECK_MODULES(PANEL, libpanelapplet-2.0, have_panel=true, have_panel=false)
if test "x${have_panel}" = "xfalse"; then
	AC_MSG_ERROR([No libpanelapplet-2.0 package information found])
fi

AC_SUBST(PANEL_CFLAGS)
AC_SUBST(PANEL_LIBS)

PKG_CHECK_MODULES(CSPI, cspi-1.0, have_spi=true, have_spi=false)
if test "x${have_spi}" = "xfalse"; then
	AC_MSG_ERROR([No cspi-1.0 package information found])
fi

AC_SUBST(CSPI_CFLAGS)
AC_SUBST(CSPI_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-glib-1, have_dbus=true, have_dbus=false)
if test "x${have_dbus}" = "xfalse"; then
	AC_MSG_ERROR([No dbus-glib-1 package information found])
fi

AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(GNOME, libgnome-2.0, have_gnome=true, have_gnome=false)
if test "x${have_gnome}" = "xfalse"; then
	AC_MSG_ERROR([No libgnome-2.0 package information found])
fi

AC_SUBST(GNOME_CFLAGS)
AC_SUBST(GNOME_LIBS)

PKG_CHECK_MODULES(GNOMEUI, libgnomeui-2.0, have_gnomeui=true, have_gnomeui=false)
if test "x${have_gnomeui}" = "xfalse"; then
	AC_MSG_ERROR([No libgnomeui-2.0 package information found])
fi

AC_SUBST(GNOMEUI_CFLAGS)
AC_SUBST(GNOMEUI_LIBS)

PKG_CHECK_MODULES(XFIXES, xfixes, have_xfixes=true, have_xfixes=false)
if test "x${have_xfixes}" = "xfalse"; then
	AC_MSG_ERROR([No xfixes package information found])
fi

AC_SUBST(XFIXES_CFLAGS)
AC_SUBST(XFIXES_LIBS)

dnl *************
dnl          dirs
dnl *************

icondir=$datadir/pixmaps
AC_SUBST(icondir)

gladedir=$datadir/mousetweaks
AC_SUBST(gladedir)

AC_OUTPUT([Makefile
	   src/Makefile
	   data/Makefile
	   pixmaps/Makefile
	   po/Makefile.in
	   help/Makefile
	   man/Makefile])

echo ""
echo "Building panel applets:"
echo "  pointer-capture: $build_pca"
echo "  dwell-click:     $build_cta"
echo ""
