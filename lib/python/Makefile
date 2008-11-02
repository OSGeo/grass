MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

default: $(ETC)/python
	$(MAKE) $(ETC)/python/grass.py $(ETC)/python/README.txt

$(ETC)/python:
	-if [ ! -d $@ ]; then $(MKDIR) $@; fi

$(ETC)/python/%: %
	$(INSTALL_DATA) $< $@
