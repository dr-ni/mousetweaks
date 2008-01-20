if BUILD_PCA
pca_schemas = pointer-capture-applet.schemas
pca_glade   = pointer-capture-applet.glade
pca_server  = PointerCapture_Factory.server.in
pca_menu    = PointerCapture.xml
endif

if BUILD_CTA
cta_glade  = dwell-click-applet.glade
cta_server = DwellClick_Factory.server.in
cta_menu   = DwellClick.xml
endif

serverdir = $(libdir)/bonobo/servers
server_in_files = $(pca_server)	$(cta_server)
server_in_in_files = $(server_in_files:.server.in=.server.in.in)
server_DATA = $(server_in_files:.server.in=.server)

if BUILD_PCA
$(pca_server): $(pca_server).in Makefile
	sed -e "s|\@bindir\@|$(bindir)|g" < $< > $@
endif

if BUILD_CTA
$(cta_server): $(cta_server).in Makefile
	sed -e "s|\@bindir\@|$(bindir)|g" < $< > $@
endif

@INTLTOOL_SERVER_RULE@

xmldir = $(datadir)/mousetweaks
xml_DATA = ctw.glade $(pca_glade) $(cta_glade)

@INTLTOOL_DESKTOP_RULE@

uidir = $(datadir)/mousetweaks
ui_DATA = $(pca_menu) $(cta_menu)

schemasdir = @GCONF_SCHEMA_FILE_DIR@
schemas_DATA = mousetweaks.schemas $(pca_schemas)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule mousetweaks.schemas
if BUILD_PCA
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule $(pca_schemas)
endif

uninstall-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule mousetweaks.schemas
if BUILD_PCA
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule $(pca_schemas)
endif
endif

EXTRA_DIST =		      \
	$(xml_DATA)	      \
	$(schemas_DATA)       \
	$(server_in_in_files) \
	$(ui_DATA)

DISTCLEANFILES =           \
	$(server_DATA)     \
	$(server_in_files)
