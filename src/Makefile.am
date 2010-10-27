AM_CPPFLAGS =                               \
    $(WARN_CFLAGS)                          \
    -DDATADIR=\"$(datadir)/mousetweaks\"    \
    -DLOCALEDIR=\"$(datadir)/locale\"

if BUILD_PCA
pca_bin = pointer-capture-applet
endif

if BUILD_DCA 
dca_bin = dwell-click-applet
endif

bin_PROGRAMS = mousetweaks $(pca_bin) $(dca_bin)

mousetweaks_SOURCES =       \
    $(BUILT_SOURCES)        \
    mt-main.c               \
    mt-common.c             \
    mt-common.h             \
    mt-pidfile.c            \
    mt-pidfile.h            \
    mt-service.c            \
    mt-service.h            \
    mt-ctw.c                \
    mt-ctw.h                \
    mt-timer.c              \
    mt-timer.h              \
    mt-cursor.c             \
    mt-cursor.h             \
    mt-cursor-manager.c     \
    mt-cursor-manager.h     \
    mt-listener.c           \
    mt-listener.h           \
    mt-sig-handler.c        \
    mt-sig-handler.h        \
    mt-settings.c           \
    mt-settings.h

mousetweaks_CFLAGS =        \
    $(AM_CPPFLAGS)          \
    $(DEPENDENCIES_CFLAGS)

mousetweaks_LDADD =         \
    $(DEPENDENCIES_LIBS)

dwell_click_applet_SOURCES =    \
    $(BUILT_SOURCES)            \
    dwell-click-applet.c        \
    mt-common.c                 \
    mt-common.h

dwell_click_applet_CFLAGS =     \
    $(AM_CPPFLAGS)              \
    $(PANELAPPLET_CFLAGS)

dwell_click_applet_LDADD =      \
    $(PANELAPPLET_LIBS)

pointer_capture_applet_SOURCES =    \
    pointer-capture-applet.c        \
    mt-common.c                     \
    mt-common.h

pointer_capture_applet_CFLAGS =     \
    $(AM_CPPFLAGS)                  \
    $(PANELAPPLET_CFLAGS)

pointer_capture_applet_LDADD =      \
    $(PANELAPPLET_LIBS)

BUILT_SOURCES =                     \
    mt-enum-types.c                 \
    mt-enum-types.h

EXTRA_DIST =                        \
    mt-enum-types.c.template        \
    mt-enum-types.h.template

CLEANFILES =                        \
    stamp-mt-enum-types.h           \
    $(BUILT_SOURCES)

# GLib mkenum
mt-enum-types.h: stamp-mt-enum-types.h
	@true

stamp-mt-enum-types.h: mt-enum-types.h.template mt-common.h
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter-out $<,$^) > xgen-gtbh \
	&& (cmp -s xgen-gtbh mt-enum-types.h || cp xgen-gtbh mt-enum-types.h) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)

mt-enum-types.c: mt-enum-types.c.template mt-common.h
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter-out $<,$^) > xgen-gtbc \
	&& (cmp -s xgen-gtbc mt-enum-types.c || cp xgen-gtbc mt-enum-types.c) \
	&& rm -f xgen-gtbc

-include $(top_srcdir)/git.mk
