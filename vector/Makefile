
MODULE_TOPDIR = ..

SUBDIRS = \
	v.buffer \
	v.build \
	v.build.polylines \
	v.category \
	v.clean \
	v.convert \
	v.db.connect \
	v.db.select \
	v.distance \
	v.drape \
	v.edit \
	v.extract \
	v.extrude \
	v.generalize \
	v.hull \
	v.info \
	v.in.ascii \
	v.in.db \
	v.in.dxf \
	v.in.region \
	v.in.sites \
	v.kcv \
	v.kernel \
	v.label \
	v.lrs \
	v.proj \
	v.mkgrid \
	v.neighbors \
	v.net \
	v.net.alloc \
	v.net.iso \
	v.net.path \
	v.net.salesman \
	v.net.steiner \
	v.net.visibility \
	v.normal \
	v.out.ascii \
	v.out.dxf \
	v.out.pov \
	v.out.svg \
	v.out.vtk \
	v.overlay \
	v.parallel \
	v.patch \
	v.perturb \
	v.split \
	v.qcount \
	v.random \
	v.reclass \
	v.sample \
	v.segment \
	v.select \
	v.support \
	v.surf.idw \
	v.surf.rst \
	v.transform \
	v.to.db \
	v.to.points \
	v.to.rast \
	v.to.rast3 \
	v.type \
	v.univar \
	v.voronoi \
	v.what \
	v.what.rast\
	v.vol.rst \
	lidar

include $(MODULE_TOPDIR)/include/Make/Platform.make

OGRBASED = v.out.ogr v.in.ogr v.external

#compile if OGR present:
ifneq ($(USE_OGR),)
    SUBDIRS += $(OGRBASED)
endif

TCLTKBASED = v.digit

#compile if Tcl/Tk present:
ifneq ($(strip $(TCLTKLIBS)),)
    SUBDIRS += $(TCLTKBASED)
endif


OPENDWGBASED = v.in.dwg

#compile if OPENDWG present:
ifneq ($(USE_OPENDWG),)
    SUBDIRS += $(OPENDWGBASED)
endif

PGM = vectorintro

include $(MODULE_TOPDIR)/include/Make/Dir.make

default: htmldir

htmldir: parsubdirs

clean: cleansubdirs
