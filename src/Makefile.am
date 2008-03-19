INCLUDES =				     \
	$(DEPENDENCIES_CFLAGS)		     \
	-DDATADIR=\"$(datadir)/mousetweaks\" \
	-DPIXMAPDIR=\"$(datadir)/pixmaps\"   \
	-DLOCALEDIR=\"$(datadir)/locale\"

if BUILD_PCA
pca_bin = pointer-capture-applet
endif

if BUILD_DCA 
dca_bin = dwell-click-applet
endif

bin_PROGRAMS = mousetweaks $(pca_bin) $(dca_bin)

mousetweaks_SOURCES =	\
	mt-main.c	\
	mt-main.h	\
	mt-common.c	\
	mt-common.h	\
	mt-pidfile.c	\
	mt-pidfile.h	\
	mt-dbus.c	\
	mt-dbus.h	\
	mt-ctw.c	\
	mt-ctw.h	\
	mt-timer.c	\
	mt-timer.h

mousetweaks_LDFLAGS = $(DEPENDENCIES_LIBS)

dwell_click_applet_SOURCES = \
	dwell-click-applet.c \
	mt-common.c	     \
	mt-common.h

dwell_click_applet_LDFLAGS = $(DEPENDENCIES_LIBS)

pointer_capture_applet_SOURCES = \
	pointer-capture-applet.c \
	mt-common.c		 \
	mt-common.h

pointer_capture_applet_LDFLAGS = $(DEPENDENCIES_LIBS)
