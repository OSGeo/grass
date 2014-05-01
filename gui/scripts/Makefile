MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Python.make

DSTDIR = $(GUIDIR)/scripts

ifdef MINGW
PYFILES := $(patsubst %.py, $(GUIDIR)/%.py, $(filter %.py, $(PY_SOURCES)))
else
PYFILES := $(patsubst %.py, $(GUIDIR)/%, $(filter %.py, $(PY_SOURCES)))
endif

default: $(PYFILES)

$(GUIDIR)/%: %.py | $(GUIDIR)
	$(INSTALL) $< $@

$(GUIDIR)/%.py: %.py | $(GUIDIR)
	$(INSTALL) $< $@

$(GUIDIR):
	-test -d $@ | $(MKDIR) $@
