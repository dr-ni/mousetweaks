if BUILD_PCA
pca_schemas = pointer-capture-applet.schemas.in
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

@INTLTOOL_SCHEMAS_RULE@
schemasdir = @GCONF_SCHEMA_FILE_DIR@
schemas_in_files = mousetweaks.schemas.in $(pca_schemas)
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule mousetweaks.schemas
if BUILD_PCA
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-install-rule pointer-capture-applet.schemas
endif

uninstall-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule mousetweaks.schemas
if BUILD_PCA
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	$(GCONFTOOL) --makefile-uninstall-rule pointer-capture-applet.schemas
endif
endif

EXTRA_DIST =		      \
	$(ui_DATA)	      \
	$(schemas_in_files)   \
	$(server_in_in_files) \
	$(menu_DATA)

DISTCLEANFILES =           \
	$(server_DATA)     \
	$(server_in_files) \
	$(schemas_DATA)

-include $(top_srcdir)/git.mk
