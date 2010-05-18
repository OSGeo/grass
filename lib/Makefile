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
	btree2 \
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

default: subdirs

