MODULE_TOPDIR = ../..

PGM = r.in.wms

include $(MODULE_TOPDIR)/include/Make/Script.make
include $(MODULE_TOPDIR)/include/Make/Python.make

MODULES = wms_base wms_drv wms_gdal_drv wms_cap_parsers
PYFILES := $(patsubst %,$(ETC)/%.py,$(MODULES))
PYCFILES := $(patsubst %,$(ETC)/%.pyc,$(MODULES))

default: script $(PYFILES) $(PYCFILES)

$(ETCDIR):
	$(MKDIR) $@

$(ETC)/%: % | $(ETC)
	$(INSTALL_DATA) $< $@
