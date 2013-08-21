MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Python.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/temporal

MODULES = base core abstract_dataset abstract_map_dataset abstract_space_time_dataset space_time_datasets open factory gui_support list register sampling metadata spatial_extent temporal_extent datetime_math temporal_granularity spatio_temporal_relationships unit_tests aggregation stds_export stds_import extract mapcalc univar_statistics temporal_topology_dataset_connector spatial_topology_dataset_connector

PYFILES := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__)
PYCFILES := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__)

default: $(PYFILES) $(PYCFILES) $(GDIR)/__init__.py $(GDIR)/__init__.pyc

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

$(DSTDIR)/%: % | $(DSTDIR)
	$(INSTALL_DATA) $< $@

#doxygen:
DOXNAME = pythontemporal
