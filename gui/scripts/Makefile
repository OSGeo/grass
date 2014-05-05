MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(GUIDIR)/scripts

ifdef MINGW
PYFILES := $(patsubst %.py, $(DSTDIR)/%.py, $(filter %.py, $(PY_SOURCES)))
else
PYFILES := $(patsubst %.py, $(DSTDIR)/%, $(filter %.py, $(PY_SOURCES)))
endif

default: $(PYFILES)

$(DSTDIR)/%: %.py | $(DSTDIR)
	$(INSTALL) $< $@

$(DSTDIR)/%.py: %.py | $(DSTDIR)
	$(INSTALL) $< $@

$(DSTDIR):
	-test -d $@ | $(MKDIR) $@
