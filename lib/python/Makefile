MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

default: $(ETC)/python

$(ETC)/python: *.py
	if [ ! -d $@ ]; then $(MKDIR) $@; fi
	for file in $^ ; do $(INSTALL_DATA) $$file $@ ; done

clean:
