MODULE_TOPDIR = ..

SUBDIRS = \
	i.ask \
	i.atcorr \
	i.cluster \
	i.find \
	i.gensig \
	i.gensigset \
	i.group \
	i.his.rgb \
	i.maxlik \
	i.rectify \
	i.rgb.his \
	i.smap \
	i.target \
	i.pca \
	i.cca

include $(MODULE_TOPDIR)/include/Make/Platform.make

FFTWBASED = i.fft i.ifft i.zc

XMONBASED = i.class i.ortho.photo i.points i.vpoints

#compile if FFTW present:
ifneq ($(strip $(FFTWLIB)),)
    SUBDIRS += $(FFTWBASED)
endif

#compile if interactive Xmons are present:
ifneq ($(USE_X11),)
    SUBDIRS += $(XMONBASED)
endif

PGM = imageryintro

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: htmldir

htmldir: parsubdirs

clean: cleansubdirs
