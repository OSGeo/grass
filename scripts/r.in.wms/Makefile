MODULE_TOPDIR = ../..

PGM = r.in.wms

MODULES = wms_base wms_drv wms_gdal_drv
PYFILES := $(patsubst %,$(ETC)/%.py,$(MODULES))
PYCFILES := $(patsubst %,$(ETC)/%.pyc,$(MODULES))

include $(MODULE_TOPDIR)/include/Make/Script.make
include $(MODULE_TOPDIR)/include/Make/Python.make

default: script $(PYFILES) $(PYCFILES)

$(ETCDIR):
	$(MKDIR) $@

$(ETC)/%: % | $(ETC)
	$(INSTALL_DATA) $< $@
