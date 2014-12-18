MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(GUIDIR)/scripts

ifdef MINGW
PYFILES := $(patsubst %.py, $(DSTDIR)/%_wrapper.py, $(filter %.py, $(PY_SOURCES)))
BATFILES := $(patsubst %.py, $(DSTDIR)/%.bat, $(filter %.py, $(PY_SOURCES)))
$(DSTDIR)/%_wrapper.py: %.py | $(DSTDIR)
	$(INSTALL) $< $@
$(DSTDIR)/%.bat: $(MODULE_TOPDIR)/scripts/windows_launch.bat
	sed -e "s#SCRIPT_NAME#$(*)_wrapper#" -e "s#SCRIPT_DIR#%GISBASE%/gui/scripts#" $(MODULE_TOPDIR)/scripts/windows_launch.bat > $@
	unix2dos $@
default: $(PYFILES) $(BATFILES)
else
PYFILES := $(patsubst %.py, $(DSTDIR)/%, $(filter %.py, $(PY_SOURCES)))
$(DSTDIR)/%: %.py | $(DSTDIR)
	$(INSTALL) $< $@
default: $(PYFILES)
endif

$(DSTDIR):
	-test -d $@ | $(MKDIR) $@
