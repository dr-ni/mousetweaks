AC_PREREQ([2.64])
AC_INIT([mousetweaks],
        [3.12.0],
        [https://bugzilla.gnome.org/enter_bug.cgi?product=mousetweaks],
        [mousetweaks],
        [https://live.gnome.org/Mousetweaks/Home])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PREFIX_DEFAULT([/usr])

AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-bzip2 -Wno-portability])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

AC_PROG_CC
AC_PROG_INSTALL
AC_HEADER_STDC
AC_C_CONST
AM_PROG_CC_C_O

dnl *** GNOME macros ***

GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

dnl *** i18n ***

IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=mousetweaks
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext Package])
AM_GLIB_GNU_GETTEXT

dnl *** gsettings ***

GLIB_GSETTINGS

dnl *** dependencies ***

GLIB_REQUIRED=2.25.3
GIO_REQUIRED=2.25.9
GTK_REQUIRED=3.0.0
GDS_REQUIRED=0.1.0

PKG_CHECK_MODULES(DEPENDENCIES,
    glib-2.0 >= $GLIB_REQUIRED
    gio-2.0 >= $GIO_REQUIRED
    gtk+-3.0 >= $GTK_REQUIRED
    gsettings-desktop-schemas >= $GDS_REQUIRED
    x11
    xcursor
    xfixes
    xtst)

dnl *** GLib tools ***

GLIB_MKENUMS="$($PKG_CONFIG --variable=glib_mkenums glib-2.0)"
AC_SUBST([GLIB_MKENUMS])

dnl *** output ***

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    data/Makefile
    pixmaps/Makefile
    po/Makefile.in
    man/Makefile
])

AC_OUTPUT

