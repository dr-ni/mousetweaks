uidir = $(datadir)/mousetweaks
ui_DATA = mousetweaks.ui

convertdir = $(datadir)/GConf/gsettings
convert_DATA = mousetweaks.convert

gsettings_ENUM_NAMESPACE = org.gnome.mousetweaks
gsettings_ENUM_FILES = $(top_srcdir)/src/mt-common.h

gsettings_SCHEMAS = org.gnome.mousetweaks.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST =                \
    $(ui_DATA)              \
    $(gsettings_SCHEMAS)   \
    $(gsettings_ENUM_FILES) \
    $(convert_DATA)

-include $(top_srcdir)/git.mk
