MODULE_TOPDIR = ../../..

PACKAGE = "grasslibs"

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

MODULES = date gis raster gmath proj imagery vector rtree display stats \
	dbmi raster3d arraystats cluster vedit ogsf nviz segment rowio

date_LIBS       = $(DATETIMELIB)
gis_LIBS        = $(GISLIB)
raster_LIBS     = $(RASTERLIB)
gmath_LIBS      = $(GMATHLIB)
proj_LIBS       = $(GPROJLIB)
imagery_LIBS    = $(IMAGERYLIB)
vector_LIBS     = $(VECTORLIB)
rtree_LIBS      = $(RTREELIB)
display_LIBS    = $(DISPLAYLIB)
stats_LIBS      = $(STATSLIB)
dbmi_LIBS       = $(DBMILIB)
raster3d_LIBS   = $(RASTER3DLIB)
arraystats_LIBS = $(ARRAYSTATSLIB)
cluster_LIBS    = $(CLUSTERLIB)
vedit_LIBS      = $(VEDITLIB)
ogsf_LIBS       = $(OGSFLIB)
nviz_LIBS       = $(NVIZLIB)
segment_LIBS	= $(SEGMENTLIB)
rowio_LIBS	= $(ROWIOLIB)

date_HDRS       = datetime.h defs/datetime.h
gis_HDRS        = gis.h defs/gis.h
raster_HDRS     = raster.h defs/raster.h
gmath_HDRS      = gmath.h defs/gmath.h
proj_HDRS       = gprojects.h defs/gprojects.h
imagery_HDRS    = imagery.h defs/imagery.h
vector_HDRS     = vector.h defs/vector.h vect/dig_structs.h vect/dig_defines.h vect/dig_externs.h 
rtree_HDRS	= rtree.h
display_HDRS    = display.h defs/display.h
stats_HDRS      = stats.h defs/stats.h
dbmi_HDRS       = dbmi.h defs/dbmi.h
raster3d_HDRS   = raster3d.h defs/raster3d.h
arraystats_HDRS = arraystats.h defs/arraystats.h
cluster_HDRS    = cluster.h defs/cluster.h
vedit_HDRS      = vedit.h defs/vedit.h
ogsf_HDRS       = ogsf.h defs/ogsf.h
nviz_HDRS       = nviz.h defs/nviz.h
segment_HDRS    = segment.h defs/segment.h
rowio_HDRS      = rowio.h defs/rowio.h

proj_INC        = $(PROJINC)
vector_INC      = $(VECT_INC) $(VECT_CFLAGS)
vedit_INC       = $(VECT_INC) $(VECT_CFLAGS)

SED = sed
CTYPESGEN = ./ctypesgen.py
CTYPESFLAGS = --cpp "$(CC) -E $(CPPFLAGS) $(LFS_CFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(DEFS) $(EXTRA_INC) $(INC) -D__GLIBC_HAVE_LONG_LONG"
EXTRA_CLEAN_FILES := $(wildcard ctypesgencore/*.pyc) $(wildcard ctypesgencore/*/*.pyc)

ifneq ($(MINGW),)
INTLLIB = -lintl3
endif

include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/lib

PYFILES  := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__ ctypes_preamble ctypes_loader)
PYCFILES  := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__ ctypes_preamble ctypes_loader)
LPYFILES := $(patsubst %,$(OBJDIR)/%.py,$(MODULES))

ifeq ($(strip $(GRASS_LIBRARY_TYPE)),shlib)
default:
	$(MAKE) $(DSTDIR)
	$(MAKE) $(LPYFILES) $(PYFILES) $(PYCFILES)
else
default:
	@echo "Cannot build Python wrappers for static libraries" >&2
	exit 1
endif

$(DSTDIR)/__init__.py: __init__.py | $(DSTDIR)
	$(INSTALL_DATA) $< $@

$(DSTDIR)/%.py: $(OBJDIR)/%.py | $(DSTDIR)
	$(SED) -f fix.sed $< > $@

$(DSTDIR)/ctypes_%.py: %.py | $(DSTDIR)
	$(INSTALL_DATA) $< $@

define module_rule
$(1)_DEPS = $$(patsubst %.h,$(ARCH_INCDIR)/%.h,$$($(1)_HDRS))
$(OBJDIR)/$(1).py: $$($(1)_DEPS) | $(OBJDIR)
	$$(call run_grass,$(CTYPESGEN) $(CTYPESFLAGS) $$($(1)_LIBS) $$($(1)_INC) $(EXTRA_LIBS) $$($(1)_DEPS) -o $$@)
endef
$(foreach module,$(MODULES),$(eval $(call module_rule,$(module))))

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

.SECONDARY: $(patsubst %,%.py,$(MODULES))
