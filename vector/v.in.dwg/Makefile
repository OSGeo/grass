MODULE_TOPDIR = ../..

PGM=v.in.dwg
ADINIT = $(GISBASE)/etc/adinit.dat

DEPENDENCIES = $(VECTDEP) $(DBMIDEP) $(GISDEP)
LIBES     = $(VECTLIB) $(DBMILIB) $(GISLIB) $(OPENDWGLIBPATH) $(OPENDWGLIB)
EXTRA_INC = $(VECT_INC) $(OPENDWGINCPATH)
EXTRA_CFLAGS = $(VECT_CFLAGS)
 
include $(MODULE_TOPDIR)/include/Make/Module.make

default: chck cmd $(ADINIT)

chck:
	./check

#a bit ugly:
$(ADINIT):
	$(INSTALL_DATA) `echo $(OPENDWGINCPATH) | cut -b3-`/adinit/adinit.dat $(ADINIT)
