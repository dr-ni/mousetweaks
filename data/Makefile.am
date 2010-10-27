if BUILD_PCA
pca_schema  = org.gnome.pointer-capture.gschema.xml.in
pca_ui      = pointer-capture-applet.ui
pca_server  = PointerCapture_Factory.server.in
pca_menu    = PointerCapture.xml
endif

if BUILD_DCA
dca_ui     = dwell-click-applet.ui
dca_server = DwellClick_Factory.server.in
dca_menu   = DwellClick.xml
endif

@INTLTOOL_SERVER_RULE@
serverdir = $(libdir)/bonobo/servers
server_in_files = $(pca_server)	$(dca_server)
server_in_in_files = $(server_in_files:.server.in=.server.in.in)
server_DATA = $(server_in_files:.server.in=.server)

if BUILD_PCA
$(pca_server): $(pca_server).in Makefile
	sed -e "s|\@bindir\@|$(bindir)|g" < $< > $@
endif

if BUILD_DCA
$(dca_server): $(dca_server).in Makefile
	sed -e "s|\@bindir\@|$(bindir)|g" < $< > $@
endif

uidir = $(datadir)/mousetweaks
ui_DATA = mousetweaks.ui $(pca_ui) $(dca_ui)

menudir = $(datadir)/mousetweaks
menu_DATA = $(pca_menu) $(dca_menu)

convertdir = $(datadir)/GConf/gsettings
convert_DATA = mousetweaks.convert

gsettings_ENUM_NAMESPACE = org.gnome.mousetweaks
gsettings_ENUM_FILES = $(top_srcdir)/src/mt-common.h

gsettings_in_files = org.gnome.mousetweaks.gschema.xml.in $(pca_schema)
gsettings_SCHEMAS = $(gsettings_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST =                \
    $(server_in_in_files)   \
    $(menu_DATA)            \
    $(ui_DATA)              \
    $(gsettings_in_files)   \
    $(gsettings_ENUM_FILES) \
    $(convert_DATA)

DISTCLEANFILES =            \
    $(server_DATA)          \
    $(server_in_files)      \
    $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
