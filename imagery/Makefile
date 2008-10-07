MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make

SUBDIRS = \
	i.albedo \
	i.atcorr \
	i.cca \
	i.cluster \
	i.find \
	i.gensig \
	i.gensigset \
	i.group \
	i.his.rgb \
	i.maxlik \
	i.modis.qc \
	i.rectify \
	i.rgb.his \
	i.smap \
	i.target \
	i.pca \
	i.vi 

FFTWBASED = i.fft i.ifft i.zc

#compile if FFTW present:
ifneq ($(strip $(FFTWLIB)),)
    SUBDIRS += $(FFTWBASED)
endif

PGM = imageryintro

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: htmldir

htmldir: parsubdirs

