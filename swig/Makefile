MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make

#compile if PYTHON present:
ifneq ($(USE_PYTHON),)
ifneq ($(strip $(CXX)),)
    SUBDIRS += python
endif
endif

# doxygen:
DOXNAME=swig

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default: parsubdirs
