MODULE_TOPDIR = ../../..

PACKAGE = "grasslibs"

include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# doxygen:
DOXNAME=
DOXINPUT=grasspython.dox

MODULES = date grass raster gmath proj imagery vector display stats \
	dbmi g3d arraystats cluster trans vedit ogsf nviz

date_LIBS       = $(DATETIMELIB)
grass_LIBS      = $(GISLIB)
raster_LIBS     = $(RASTERLIB)
gmath_LIBS      = $(GMATHLIB)
proj_LIBS       = $(GPROJLIB)
imagery_LIBS    = $(IMAGERYLIB)
vector_LIBS     = $(VECTLIB)
display_LIBS    = $(DISPLAYLIB)
stats_LIBS      = $(STATSLIB)
dbmi_LIBS       = $(DBMILIB)
g3d_LIBS        = $(G3DLIB)
arraystats_LIBS = $(ARRAYSTATSLIB)
cluster_LIBS    = $(CLUSTERLIB)
trans_LIBS      = $(TRANSLIB)
vedit_LIBS      = $(VEDITLIB)
ogsf_LIBS       = $(OGSFLIB)
nviz_LIBS       = $(NVIZLIB)

date_HDRS       = datetime.h P_datetime.h
grass_HDRS      = gis.h gisdefs.h
raster_HDRS     = raster.h rasterdefs.h
gmath_HDRS      = gmath.h
proj_HDRS       = gprojects.h
imagery_HDRS    = imagery.h imagedefs.h
vector_HDRS     = vector.h vect/dig_structs.h vect/dig_defines.h
display_HDRS    = display.h
stats_HDRS      = stats.h
dbmi_HDRS       = dbmi.h
g3d_HDRS        = G3d.h
arraystats_HDRS = arraystats.h
cluster_HDRS    = cluster.h
trans_HDRS      = transform.h
vedit_HDRS      = vedit.h
ogsf_HDRS       = ogsf_proto.h gstypes.h gsurf.h kftypes.h keyframe.h
nviz_HDRS       = nviz.h

proj_INC        = $(PROJINC)
vector_INC      = $(VECT_INC)
vedit_INC       = $(VECT_INC)

SED = sed
CTYPESGEN = ./ctypesgen.py
CTYPESFLAGS = --cpp "$(CC) -E $(LFS_CFLAGS) $(EXTRA_CFLAGS) $(NLS_CFLAGS) $(DEFS) $(EXTRA_INC) $(INC)"
EXTRA_CLEAN_FILES := $(foreach M,$(MODULES),$(M).py)

ifneq ($(MINGW),)
EXTRA_LIBS = $(INTLLIB)
endif

include $(MODULE_TOPDIR)/include/Make/Python.make

PYDIR = $(ETC)/python
GDIR = $(PYDIR)/grass
DSTDIR = $(GDIR)/lib

PYFILES  := $(patsubst %,$(DSTDIR)/%.py,$(MODULES) __init__ ctypes_preamble ctypes_loader)
PYCFILES  := $(patsubst %,$(DSTDIR)/%.pyc,$(MODULES) __init__ ctypes_preamble ctypes_loader)
LPYFILES := $(patsubst %,%.py,$(MODULES))

ifneq ($(strip $(CTYPESGEN)),)
default:
	$(MAKE) $(DSTDIR)
	$(MAKE) $(LPYFILES) $(PYFILES) $(PYCFILES)
endif

$(DSTDIR)/%.py: %.py | $(DSTDIR)
	$(SED) -f fix.sed $< > $@

$(DSTDIR)/ctypes_%.py: %.py | $(DSTDIR)
	$(INSTALL_DATA) $< $@

%.py: $(%_HDRS) $(%_LIBS)
	$(call run_grass,$(CTYPESGEN) $(CTYPESFLAGS) $($*_LIBS) $($*_INC) $(EXTRA_LIBS) $(patsubst %.h,$(ARCH_INCDIR)/%.h,$($*_HDRS)) -o $@)

$(PYDIR):
	$(MKDIR) $@

$(GDIR): | $(PYDIR)
	$(MKDIR) $@

$(DSTDIR): | $(GDIR)
	$(MKDIR) $@

.SECONDARY: $(patsubst %,%.py,$(MODULES))
