MODULE_TOPDIR = ../..

PGM=ps.map
PROLOG=$(ETC)/paint/prolog.ps

LIBES     = $(VECTLIB) $(GPROJLIB) $(SYMBLIB) $(GISLIB) $(DATETIMELIB) $(IMAGERYLIB) $(GMATHLIB)
DEPENDENCIES = $(VECTDEP) $(SYMBDEP) $(GISDEP) $(DATETIDEP) $(IMAGERYDEP) $(GMATHDEP) \
               colortable.h group.h labels.h map_info.h ps_info.h vector.h \
               comment.h header.h local_proto.h

EXTRA_INC = $(VECT_INC) $(PROJINC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
 
include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(PROLOG) patt

$(PROLOG): prolog.ps
	$(MKDIR) $(ETC)/paint
	$(INSTALL_DATA) prolog.ps $(PROLOG)

patt:
	$(MKDIR) $(ETC)/paint/patterns/
	$(INSTALL_DATA) patterns/*.eps $(ETC)/paint/patterns/

