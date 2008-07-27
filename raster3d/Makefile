MODULE_TOPDIR = ..

SUBDIRS = \
	r3.cross.rast \
	r3.in.ascii \
	r3.in.v5d \
	r3.gwflow \
	r3.mkdspf \
	r3.out.ascii \
	r3.out.v5d \
	r3.out.vtk \
	r3.stats \
	r3.to.rast \
	base

include $(MODULE_TOPDIR)/include/Make/Platform.make

PGM = raster3dintro

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: htmldir

htmldir: parsubdirs

clean: cleansubdirs
