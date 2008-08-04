MODULE_TOPDIR = ..

#order is relevant:
SUBDIRS = \
	datetime \
	gis \
	gmath \
	linkm \
	driver \
	pngdriver \
	psdriver \
	raster \
	bitmap \
	btree \
	display \
	db \
	vask \
	edit \
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
	python

include $(MODULE_TOPDIR)/include/Make/Platform.make

OPENGLBASED = \
	ogsf \
	nviz

#compile if OPENGLBASED present:
ifneq ($(strip $(OPENGLLIB)),)
    SUBDIRS += $(OPENGLBASED)
endif

ifneq ($(USE_CAIRO),)
    SUBDIRS += cairodriver
endif

#compile if C++ compiler present:
ifneq ($(strip $(CXX)),)
    SUBDIRS += iostream
endif

#doxygen:
DOXNAME=grass

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: headers
	$(MAKE) subdirs

headers: 
	if [ ! -d $(GISBASE)/include ]; then $(MKDIR) $(GISBASE)/include; fi
	if [ ! -d $(GISBASE)/include/grass ]; then $(MKDIR) $(GISBASE)/include/grass; fi
	if [ ! -d $(GISBASE)/include/grass/vect ]; then $(MKDIR) $(GISBASE)/include/grass/vect; fi
	if [ ! -d $(GISBASE)/include/grass/iostream ]; then $(MKDIR) $(GISBASE)/include/grass/iostream; fi
	if [ ! -d $(GISBASE)/include/Make ]; then $(MKDIR) $(GISBASE)/include/Make; fi
	for file in ../include/*.h ; do $(INSTALL_DATA) $$file $(GISBASE)/include/grass/ ; done
	for file in ../include/vect/*.h ; do $(INSTALL_DATA) $$file $(GISBASE)/include/grass/vect/ ; done
	for file in ../include/iostream/*.h ; do $(INSTALL_DATA) $$file $(GISBASE)/include/grass/iostream/ ; done
	for file in ../include/Make/*.make ; do $(INSTALL_DATA) $$file $(GISBASE)/include/Make/ ; done

clean: cleansubdirs
