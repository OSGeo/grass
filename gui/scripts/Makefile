MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(GUIDIR)/scripts

ifdef MINGW
PYFILES := $(patsubst %.py, $(DSTDIR)/%.py, $(filter %.py, $(PY_SOURCES)))
$(DSTDIR)/%.py: %.py | $(DSTDIR)
	$(INSTALL) $< $@
else
PYFILES := $(patsubst %.py, $(DSTDIR)/%, $(filter %.py, $(PY_SOURCES)))
$(DSTDIR)/%: %.py | $(DSTDIR)
	$(INSTALL) $< $@
endif

default: $(PYFILES)

$(DSTDIR):
	-test -d $@ | $(MKDIR) $@
