INCLUDES =			\
	$(GLADE_CFLAGS)		\
	$(PANEL_CFLAGS)		\
	$(CSPI_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(XFIXES_CFLAGS)

AM_CFLAGS =				\
	-DGLADEDIR=\"$(gladedir)\"	\
	-DDATADIR=\"$(datadir)\"	\
	-DLOCALEDIR=\"$(localedir)\"

if BUILD_PCA
pca_bin = pointer-capture-applet
endif

if BUILD_CTA 
cta_bin = dwell-click-applet
endif

bin_PROGRAMS = mousetweaks $(pca_bin) $(cta_bin)
mousetweaks_SOURCES =		\
	mt-main.c		\
	mt-main.h		\
	mt-common.c		\
	mt-common.h		\
	mt-session.c		\
	mt-session.h		\
	mt-pidfile.c		\
	mt-pidfile.h		\
	mt-dbus.c		\
	mt-dbus.h		\
	mt-ctw.c		\
	mt-ctw.h		\
	mt-timer.c		\
	mt-timer.h

mousetweaks_LDFLAGS =		\
	$(GLADE_LIBS)		\
	$(CSPI_LIBS)		\
	$(DBUS_LIBS)		\
	$(GNOMEUI_LIBS)     	\
	$(XFIXES_LIBS)

dwell_click_applet_SOURCES = \
	dwell-click-applet.c \
	mt-common.c	     \
	mt-common.h

dwell_click_applet_LDFLAGS = \
	$(GLADE_LIBS)	     \
	$(PANEL_LIBS)	     \
	$(DBUS_LIBS)

pointer_capture_applet_SOURCES = \
	pointer-capture-applet.c \
	mt-common.c		 \
	mt-common.h

pointer_capture_applet_LDFLAGS = \
	$(GLADE_LIBS)		 \
	$(PANEL_LIBS)
