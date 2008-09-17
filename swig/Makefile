MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Vars.make

#compile if PYTHON present:
ifneq ($(USE_PYTHON),)
    SUBDIRS += python
endif

# doxygen:
DOXNAME=swig

include $(MODULE_TOPDIR)/include/Make/Dir.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

default:
	@echo "Please run 'make' in the individual subdirectories"
