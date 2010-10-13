if BUILD_PCA
pca_man = pointer-capture-applet.1
endif

if BUILD_DCA
dca_man = dwell-click-applet.1
endif

man_MANS =          \
    mousetweaks.1   \
    $(pca_man)      \
    $(dca_man)

EXTRA_DIST = $(man_MANS)

-include $(top_srcdir)/git.mk
