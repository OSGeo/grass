MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make

#order is relevant:
SUBDIRS = \
	datetime \
	gis \
	raster \
	external \
	gmath \
	linkm \
	driver \
	pngdriver \
	psdriver \
	htmldriver \
	cairodriver \
	bitmap \
	btree \
	display \
	db \
	fonts \
	gtcltk \
	form \
	imagery \
	cluster \
	proj \
	rowio \
	segment \
	vector \
	sites \
	rst \
	g3d \
	gpde \
	dspf \
	symbol \
	init \
	cdhc \
	stats \
	arraystats \
	python \
	ogsf \
	nviz \
	iostream \
	manage

#doxygen:
DOXNAME=grass

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

SRCH := $(wildcard ../include/*.h ../include/vect/*.h ../include/iostream/*.h)
DSTH := $(patsubst ../include/%.h,$(ARCH_INCDIR)/%.h,$(SRCH))

SRCMAKE := $(wildcard ../include/Make/*.make)
DSTMAKE := $(patsubst ../include/Make/%.make,$(ARCH_DISTDIR)/include/Make/%.make,$(SRCMAKE))

default: $(DSTH) $(DSTMAKE)
	$(MAKE) subdirs

$(ARCH_DISTDIR)/include:
	$(MKDIR) $@

$(ARCH_INCDIR): | $(ARCH_DISTDIR)/include
	$(MKDIR) $@

$(ARCH_INCDIR)/vect: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_INCDIR)/iostream: | $(ARCH_INCDIR)
	$(MKDIR) $@

$(ARCH_INCDIR)/%.h: ../include/%.h | $(ARCH_INCDIR) $(ARCH_INCDIR)/vect $(ARCH_INCDIR)/iostream
	$(INSTALL_DATA) $< $@

$(ARCH_DISTDIR)/include/Make/%.make: ../include/Make/%.make | $(ARCH_DISTDIR)/include/Make
	$(INSTALL_DATA) $< $@

$(ARCH_DISTDIR)/include/Make:
	$(MKDIR) $@
