uidir = $(datadir)/mousetweaks
ui_DATA = mousetweaks.ui

convertdir = $(datadir)/GConf/gsettings
convert_DATA = mousetweaks.convert

gsettings_ENUM_NAMESPACE = org.gnome.mousetweaks
gsettings_ENUM_FILES = $(top_srcdir)/src/mt-common.h

gsettings_in_files = org.gnome.mousetweaks.gschema.xml.in
gsettings_SCHEMAS = $(gsettings_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST =                \
    $(ui_DATA)              \
    $(gsettings_in_files)   \
    $(gsettings_ENUM_FILES) \
    $(convert_DATA)

DISTCLEANFILES =            \
    $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
