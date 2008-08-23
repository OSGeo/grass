MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

default: $(ETC)/python

$(ETC)/python: grass.py README.txt
	if [ ! -d $@ ]; then $(MKDIR) $@; fi
	for file in $^ ; do $(INSTALL_DATA) $$file $@ ; done

.PHONY: $(ETC)/python

