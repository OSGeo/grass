MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make

#order is relevant:
SUBDIRS = \
	datetime \
	gis \
	gmath \
	linkm \
	driver \
	pngdriver \
	psdriver \
	htmldriver \
	cairodriver \
	raster \
	bitmap \
	btree \
	display \
	db \
	external \
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
	nviz

#compile if C++ compiler present:
ifneq ($(strip $(CXX)),)
    SUBDIRS += iostream
endif

#doxygen:
DOXNAME=grass

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

SRCH := $(wildcard ../include/*.h ../include/vect/*.h ../include/iostream/*.h)
DSTH := $(patsubst ../include/%.h,$(ARCH_INCDIR)/%.h,$(SRCH))

SRCMAKE := $(wildcard ../include/Make/*.make)
DSTMAKE := $(patsubst ../include/Make/%.make,$(ARCH_DISTDIR)/include/Make/%.make,$(SRCMAKE))

default: headers
	$(MAKE) subdirs

headers: 
	if [ ! -d $(ARCH_DISTDIR)/include ]; then $(MKDIR) $(ARCH_DISTDIR)/include; fi
	if [ ! -d $(ARCH_INCDIR) ]; then $(MKDIR) $(ARCH_INCDIR); fi
	if [ ! -d $(ARCH_INCDIR)/vect ]; then $(MKDIR) $(ARCH_INCDIR)/vect; fi
	if [ ! -d $(ARCH_INCDIR)/iostream ]; then $(MKDIR) $(ARCH_INCDIR)/iostream; fi
	$(MAKE) $(DSTH)
	if [ ! -d $(ARCH_DISTDIR)/include/Make ]; then $(MKDIR) $(ARCH_DISTDIR)/include/Make; fi
	$(MAKE) $(DSTMAKE)

$(ARCH_INCDIR)/%.h: ../include/%.h
	$(INSTALL_DATA) $< $@

$(ARCH_DISTDIR)/include/Make/%.make: ../include/Make/%.make
	$(INSTALL_DATA) $< $@
