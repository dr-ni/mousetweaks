AM_CFLAGS =                                 \
    $(DEPENDENCIES_CFLAGS)                  \
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
    mt-main.c               \
    mt-main.h               \
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
    mt-sig-handler.h

mousetweaks_CFLAGS =        \
    $(AM_CFLAGS)

mousetweaks_LDADD =         \
    $(DEPENDENCIES_LIBS)

dwell_click_applet_SOURCES =    \
    dwell-click-applet.c        \
    mt-common.c                 \
    mt-common.h

dwell_click_applet_CFLAGS =     \
    $(PANELAPPLET_CFLAGS)       \
    $(AM_CFLAGS)

dwell_click_applet_LDADD =      \
    $(PANELAPPLET_LIBS)         \
    $(DEPENDENCIES_LIBS)

pointer_capture_applet_SOURCES =    \
    pointer-capture-applet.c        \
    mt-common.c                     \
    mt-common.h

pointer_capture_applet_CFLAGS =     \
    $(PANELAPPLET_CFLAGS)           \
    $(AM_CFLAGS)

pointer_capture_applet_LDADD =      \
    $(PANELAPPLET_LIBS)             \
    $(DEPENDENCIES_LIBS)

-include $(top_srcdir)/git.mk
