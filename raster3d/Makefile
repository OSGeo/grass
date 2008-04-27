MODULE_TOPDIR = ..

SUBDIRS = \
	r3.cross.rast \
	r3.in.ascii \
	r3.in.v5d \
	r3.gwflow \
	r3.out.ascii \
	r3.out.v5d \
	r3.out.vtk \
	r3.stats \
	r3.to.rast \
	base

OPENGLBASED = r3.mkdspf

include $(MODULE_TOPDIR)/include/Make/Platform.make

#compile if OPENGLU present:
ifneq ($(strip $(OPENGLULIB)),)
    SUBDIRS += $(OPENGLBASED)
endif

PGM = raster3dintro

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: htmldir

htmldir: parsubdirs

clean: cleansubdirs
