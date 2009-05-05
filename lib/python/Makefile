MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: $(ETC)/python
	$(MAKE) $(ETC)/python/grass.py $(ETC)/python/README.txt

$(ETC)/python:
	-if [ ! -d $@ ]; then $(MKDIR) $@; fi

$(ETC)/python/%: %
	$(INSTALL_DATA) $< $@

#doxygen:
DOXNAME=grasspython
