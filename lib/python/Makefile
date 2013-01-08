MODULE_TOPDIR = ../..

#include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python/grass

SUBDIRS = script ctypes temporal pygrass

default: $(PYDIR)/__init__.py
	$(MAKE) subdirs
#	-$(MAKE) -C script || echo $(CURDIR)/script >> $(ERRORLOG)
#	-$(MAKE) -C ctypes || echo $(CURDIR)/ctypes >> $(ERRORLOG)
#	-$(MAKE) -C temporal || echo $(CURDIR)/temporal >> $(ERRORLOG)
#	-$(MAKE) -C pygrass || echo $(CURDIR)/pygrass >> $(ERRORLOG)

$(PYDIR):
	$(MKDIR) $@

$(PYDIR)/__init__.py: __init__.py | $(PYDIR)
	$(INSTALL_DATA) $< $@

# doxygen
DOXNAME = python
