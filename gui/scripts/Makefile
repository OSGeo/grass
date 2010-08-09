MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Rules.make
include $(MODULE_TOPDIR)/include/Make/Vars.make
include $(MODULE_TOPDIR)/include/Make/Python.make

SCRIPTDIR = $(ETC)/gui/scripts

ifdef MINGW
PYFILES := $(patsubst %.py, $(SCRIPTDIR)/%.py, $(filter %.py, $(PY_SOURCES)))
else
PYFILES := $(patsubst %.py, $(SCRIPTDIR)/%, $(filter %.py, $(PY_SOURCES)))
endif

default: $(PYFILES)

$(SCRIPTDIR)/%: %.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

$(SCRIPTDIR)/%.py: %.py | $(SCRIPTDIR)
	$(INSTALL) $< $@

$(SCRIPTDIR):
	-test -d $@ | $(MKDIR) $@
