MODULE_TOPDIR = ../..

PGM = i.topo.corr

LIBES = $(GISLIB) $(RASTERLIB)
DEPENDENCIES = $(GISDEP) $(RASTERDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd

fig2pdf:
	fig2dev -L pdf -p 1 -m 0.99 terrain_correction.fig | sed 's+Times-Bold+Helvetica-Bold+g' | sed 's+Times-Roman+Helvetica+g' | sed 's+Times+Helvetica+g' | sed 's+Helvetica-BoldOblique+Helvetica-BoldItal+g' | sed 's+Helvetica-Oblique+Helvetica-ReguItal+g' | sed 's+NimbusSanL-Bold+Helvetica-Bold+g' | sed 's+Helvetica-Bold-iso+Helvetica-Bold+g' | sed 's+Helvetica-iso+Helvetica+g' | sed 's+Helvetica+Helvetica+g' > terrain_correction.pdf

