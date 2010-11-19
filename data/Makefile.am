if BUILD_PCA
pca_schema  = org.gnome.pointer-capture.gschema.xml.in
pca_ui      = pointer-capture-applet.ui
pca_service = org.gnome.panel.applet.PointerCaptureAppletFactory.service.in
pca_applet  = org.gnome.applets.PointerCaptureApplet.panel-applet.in

org.gnome.panel.applet.PointerCaptureAppletFactory.service: $(pca_service)
	$(AM_V_GEN) sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

$(pca_applet): $(pca_applet).in Makefile
	$(AM_V_GEN) sed -e "s|\@BINDIR\@|$(bindir)|" -e "s|\@VERSION\@|$(VERSION)|" $< > $@
endif

if BUILD_DCA
dca_ui      = dwell-click-applet.ui
dca_service = org.gnome.panel.applet.DwellClickAppletFactory.service.in
dca_applet  = org.gnome.applets.DwellClickApplet.panel-applet.in

org.gnome.panel.applet.DwellClickAppletFactory.service: $(dca_service)
	$(AM_V_GEN) sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

$(dca_applet): $(dca_applet).in Makefile
	$(AM_V_GEN) sed -e "s|\@BINDIR\@|$(bindir)|" -e "s|\@VERSION\@|$(VERSION)|" $< > $@
endif

appletdir = $(datadir)/gnome-panel/applets
applet_in_in_files = $(applet_in_files:.panel-applet.in=.panel-applet.in.in)
applet_in_files = $(pca_applet) $(dca_applet)
applet_DATA = $(applet_in_files:.panel-applet.in=.panel-applet)

%.panel-applet: %.panel-applet.in \
	$(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po);
	LC_ALL=C $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

servicedir = $(datadir)/dbus-1/services
service_in_files = $(pca_service) $(dca_service)
service_DATA = $(service_in_files:.service.in=.service)

uidir = $(datadir)/mousetweaks
ui_DATA = mousetweaks.ui $(pca_ui) $(dca_ui)

convertdir = $(datadir)/GConf/gsettings
convert_DATA = mousetweaks.convert

gsettings_ENUM_NAMESPACE = org.gnome.mousetweaks
gsettings_ENUM_FILES = $(top_srcdir)/src/mt-common.h

gsettings_in_files = org.gnome.mousetweaks.gschema.xml.in $(pca_schema)
gsettings_SCHEMAS = $(gsettings_in_files:.xml.in=.xml)
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

EXTRA_DIST =                \
    $(service_in_files)     \
    $(applet_in_in_files)   \
    $(ui_DATA)              \
    $(gsettings_in_files)   \
    $(gsettings_ENUM_FILES) \
    $(convert_DATA)

DISTCLEANFILES =            \
    $(service_DATA)         \
    $(applet_in_files)      \
    $(applet_DATA)          \
    $(gsettings_SCHEMAS)

-include $(top_srcdir)/git.mk
