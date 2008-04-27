MODULE_TOPDIR = ../..

PGM = g.setproj

EXTRA_CFLAGS = $(PROJINC)
LIBES        = $(GPROJLIB) $(GISLIB)
DEPENDENCIES = $(GPROJDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd $(ETC)/proj-desc.table $(ETC)/proj-parms.table $(ETC)/proj-units.table

$(ETC)/%.table: %.table
	rm -f $@
	$(INSTALL_DATA) $? $@
