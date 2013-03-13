#########################################################################
#                         Variable names
# xxxINCDIR  directory(ies) including header files (example: /usr/include)
# xxxINC  cc option(s) for include directory (example: -I/usr/inlude)
# xxxLIBDIR  directory(ies) containing library (example: /usr/lib)
# xxxLIBPATH cc option for library directory (example: -L/usr/lib)
# xxx_LIBNAME library name (example: gis)
# xxxLIB full static library path 
#        (example: /home/abc/grass63/dist.i686-pc-linux-gnu/lib/libgis.a)
# xxxDEP dependency
# 
# GRASS_xxx GRASS specific (without ARCH_xxx)
#
# ARCH_xxx platform specific dirs (without GRASS_xxx)
#
# _xxx  GRASS_xxx + ARCH_xxx
#
# ALLxxx all known for GRASS make system
#
#########################################################################

# GRASS global directories and constants
# platform specific dirs
ARCH_DISTDIR	= $(GRASS_HOME)/dist.$(ARCH)
ARCH_BINDIR     = $(GRASS_HOME)/bin.$(ARCH)

GISBASE		= $(ARCH_DISTDIR)

ERRORLOG        = $(GRASS_HOME)/error.log
# include dirs
ARCH_INCDIR     = $(ARCH_DISTDIR)/include/grass

INC		= -I$(ARCH_DISTDIR)/include -I$(GISBASE)/include
VECT_INC        = $(PQINCPATH)

# libraries
ARCH_LIBDIR     = $(ARCH_DISTDIR)/lib
BASE_LIBDIR     = $(GISBASE)/lib

LIBPATH		= -L$(ARCH_LIBDIR) -L$(BASE_LIBDIR)

# object dir
OBJDIR		= OBJ.$(ARCH)

LIB_RUNTIME_DIR = $(GISBASE)/lib

RUN_GISRC       = $(GISBASE)/demolocation/.grassrc$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

#########################################################################
# these define the various directories which contain GRASS programs
# or files used by GRASS programs

BIN             = $(ARCH_DISTDIR)/bin
ETC             = $(ARCH_DISTDIR)/etc
DRIVERDIR       = $(ARCH_DISTDIR)/driver
DBDRIVERDIR     = $(ARCH_DISTDIR)/driver/db
DOCSDIR         = $(ARCH_DISTDIR)/docs
HTMLDIR         = $(ARCH_DISTDIR)/docs/html
SCRIPTDIR       = $(ARCH_DISTDIR)/scripts
MSG_DIR         = $(ARCH_DISTDIR)/etc/msgs
MO_DIR          = $(ARCH_DISTDIR)/locale
TOOLSDIR	= $(ARCH_DISTDIR)/tools

FONTDIR         = $(ARCH_DISTDIR)/fonts

GRASS_VERSION_NUMBER  = $(GRASS_VERSION_MAJOR).$(GRASS_VERSION_MINOR).$(GRASS_VERSION_RELEASE)
GRASS_VERSION_NAME    = $(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)$(GRASS_VERSION_RELEASE)

GRASS_NAME	= grass$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

##################### other #############################################

COMPILE_FLAGS_C    = $(CPPFLAGS) $(CFLAGS) $(INCLUDE_DIRS) $(INC)
COMPILE_FLAGS_CXX  = $(CPPFLAGS) $(CXXFLAGS) $(INCLUDE_DIRS) $(INC)

# crude hack for vector LFS, LFS_FLAGS should be set by configure
ifdef USE_LARGEFILES
LFS_CFLAGS = -D_FILE_OFFSET_BITS=64
endif

LDFLAGS     =  $(LIBPATH) $(LINK_FLAGS) $(LD_SEARCH_FLAGS)
VECT_CFLAGS =  $(GDALCFLAGS) $(GEOSCFLAGS)

# Object with _fmode which must be linked to each executable on Windows
ifdef MINGW
WINDRES = windres
FMODE_OBJ = $(BASE_LIBDIR)/fmode.o
MANIFEST_OBJ = $(OBJDIR)/$(basename $(notdir $@)).manifest.res
MANIFEST = internal
LDFLAGS += -lxdr -liberty -lws2_32
endif

# lexical analyzer and default options
LFLAGS      = -s

# parser generator and default options
YFLAGS      = -d -v

MANSECT = 1
MANBASEDIR = $(ARCH_DISTDIR)/docs/man
MANDIR = $(MANBASEDIR)/man$(MANSECT)
HTML2MAN = VERSION_NUMBER=$(GRASS_VERSION_NUMBER) $(GISBASE)/tools/g.html2man.py

GDAL_LINK = $(USE_GDAL)
GDAL_DYNAMIC = 1

DEPFILE = depend.mk

##################### library switches ##################################

libs = \
	ARRAYSTATS:arraystats \
	BASIC:basic \
	BITMAP:bitmap \
	BTREE:btree \
	BTREE2:btree2 \
	CAIRODRIVER:cairodriver \
	CDHC:cdhc \
	CCMATH:ccmath \
	CLUSTER:cluster \
	COORCNV:coorcnv \
	DATETIME:datetime \
	DBDIALOG:dbdialog \
	DBMIBASE:dbmibase \
	DBMICLIENT:dbmiclient \
	DBMIDRIVER:dbmidriver \
	DBSTUBS:dbstubs \
	DIG2:dig2 \
	DIG:dig \
	DISPLAY:display \
	DLG:dlg \
	DRIVER:driver \
	DSPF:dspf \
	RASTER3D:g3d \
	GIS:gis \
	GMATH:gmath \
	GPDE:gpde \
	GPROJ:gproj \
	GRAPH:dgl \
	HTMLDRIVER:htmldriver \
	IBTREE:ibtree \
	ICON:icon \
	IMAGERY:imagery \
	INTERPDATA:interpdata \
	INTERPFL:interpfl \
	IORTHO:iortho \
	ISMAP:ismap \
	LIA:lia \
	LIDAR:lidar \
	LINKM:linkm \
	LOCK:lock \
	LRS:lrs \
	MANAGE:manage \
	NVIZ:nviz \
	OGSF:ogsf \
	OPTRI:optri \
	PNGDRIVER:pngdriver \
	PSDRIVER:psdriver \
	QTREE:qtree \
	RASTER:raster \
	RLI:rli \
	ROWIO:rowio \
	RTREE:rtree \
	SEGMENT:segment \
	SHAPE:shape \
	SIM:sim \
	SITES:sites \
	SOS:sos \
	SQLP:sqlp \
	STATS:stats \
	SYMB:symb \
	TEMPORAL:temporal \
	VECTOR:vector \
	VEDIT:vedit \
	NETA:neta \
	XDISPLAY:xdisplay \
	XGD:xgd \
	XGI:xgi \
	XPM:xpm \
	IOSTREAM:iostream


ARRAYSTATSDEPS   = $(GISLIB) $(MATHLIB)
BITMAPDEPS       = $(LINKMLIB)
BTREE2DEPS       = $(GISLIB)
CAIRODRIVERDEPS  = $(DRIVERLIB) $(GISLIB) $(CAIROLIB) $(FCLIB) $(ICONVLIB)
CDHCDEPS         = $(MATHLIB)
CLUSTERDEPS      = $(IMAGERYLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
DBMIBASEDEPS     = $(GISLIB)
DBMICLIENTDEPS   = $(DBMIBASELIB) $(GISLIB)
DBMIDRIVERDEPS   = $(DBMIBASELIB) $(DBSTUBSLIB) $(GISLIB)
DBSTUBSDEPS      = $(DBMIBASELIB) $(GISLIB)
DIG2DEPS         = $(GISLIB) $(RTREELIB) $(MATHLIB)
DISPLAYDEPS      = $(HTMLDRIVERLIB) $(PNGDRIVERLIB) $(PSDRIVERLIB) $(DRIVERLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
DRIVERDEPS       = $(GISLIB) $(FTLIB) $(ICONVLIB) $(MATHLIB)
DSPFDEPS         = $(GISLIB)
FORMDEPS         = $(DBMILIB) $(GISLIB)
RASTER3DDEPS     = $(RASTERLIB) $(GISLIB)
GISDEPS          = $(DATETIMELIB) $(ZLIBLIBPATH) $(ZLIB) $(INTLLIB) $(REGEXLIBPATH) $(REGEXLIB) $(ICONVLIB) $(PTHREADLIBPATH) $(PTHREADLIB) $(MATHLIB)
GMATHDEPS        = $(GISLIB) $(FFTWLIB) $(LAPACKLIB) $(BLASLIB) $(CCMATHLIB) $(OMPLIBPATH) $(OMPLIB)
GPDEDEPS         = $(RASTER3DLIB) $(RASTERLIB) $(GISLIB) $(GMATHLIB) $(OMPLIBPATH) $(OMPLIB) $(MATHLIB)
GPROJDEPS        = $(GISLIB) $(GDALLIBS) $(PROJLIB) $(MATHLIB)
HTMLDRIVERDEPS   = $(DRIVERLIB) $(GISLIB) $(MATHLIB)
IMAGERYDEPS      = $(GISLIB) $(MATHLIB) $(RASTERLIB) $(VECTORLIB)
INTERPFLDEPS     = $(BITMAPLIB) $(DBMILIB) $(GMATHLIB) $(INTERPDATALIB) $(QTREELIB) $(VECTORLIB) $(RASTERLIB) $(GISLIB) $(MATHLIB)
#IORTHODEPS       = $(IMAGERYLIB) $(GISLIB)
LIDARDEPS        = $(VECTORLIB) $(DBMILIB) $(GMATHLIB) $(RASTERLIB) $(SEGMENTLIB) $(GISLIB) $(MATHLIB)
LRSDEPS          = $(DBMILIB) $(GISLIB)
MANAGEDEPS       = $(VECTORLIB) $(GISLIB)
NVIZDEPS         = $(OGSFLIB) $(GISLIB) $(OPENGLLIB)
OGSFDEPS         = $(BITMAPLIB) $(RASTER3DLIB) $(VECTORLIB) $(DBMILIB) $(RASTERLIB) $(GISLIB) $(FFMPEGLIBPATH) $(FFMPEGLIB) $(TIFFLIBPATH) $(TIFFLIB) $(OPENGLLIB) $(OPENGLULIB) $(MATHLIB)
PNGDRIVERDEPS    = $(DRIVERLIB) $(GISLIB) $(PNGLIB) $(MATHLIB)
PSDRIVERDEPS     = $(DRIVERLIB) $(GISLIB) $(MATHLIB)
RASTERDEPS       = $(GISLIB) $(MATHLIB)
RLIDEPS          = $(RASTERLIB) $(GISLIB) $(MATHLIB)
ROWIODEPS        = $(GISLIB)
RTREEDEPS        = $(GISLIB) $(MATHLIB)
SEGMENTDEPS      = $(GISLIB)
SIMDEPS          = $(VECTORLIB) $(RASTERLIB)
SITESDEPS        = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(DATETIMELIB)
STATSDEPS        = $(RASTERLIB) $(GISLIB) $(MATHLIB)
SYMBDEPS         = $(GISLIB) $(MATHLIB)
TEMPORALDEPS     = $(DBMILIB) $(GISLIB) $(DATETIMELIB)
VECTORDEPS       = $(DBMILIB) $(GRAPHLIB) $(DIG2LIB) $(LINKMLIB) $(RTREELIB) $(GISLIB) $(GEOSLIBS) $(GDALLIBS) $(MATHLIB) $(BTREE2LIB) $(GPROJLIB) $(RASTERLIB) $(PQLIBPATH) $(PQLIB)
VEDITDEPS        = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(MATHLIB)
NETADEPS         = $(VECTORLIB) $(DBMILIB) $(GISLIB)

ifneq ($(USE_X11),)
CAIRODRIVERDEPS += $(XLIBPATH) $(XLIB) $(XEXTRALIBS)
endif

ifneq ($(USE_CAIRO),)
DISPLAYDEPS += $(CAIRODRIVERLIB) 
endif

ifneq ($(GDAL_LINK),)
ifneq ($(GDAL_DYNAMIC),)
ifneq ($(MINGW),)
RASTERDEPS += -lkernel32
else
RASTERDEPS += $(DLLIB)
endif
else
RASTERDEPS += $(GDALLIBS)
endif
endif

ifeq ($(OPENGL_WINDOWS),1)
NVIZDEPS += -lgdi32
endif
ifneq ($(OPENGL_X11),)
NVIZDEPS += $(XLIBPATH) $(XLIB) $(XEXTRALIBS)
endif

ifeq ($(GRASS_LIBRARY_TYPE),stlib)
NEED_DEPS = 1
LIB_PREFIX = $(STLIB_PREFIX)
LIB_SUFFIX = $(STLIB_SUFFIX)
else
LIB_PREFIX = $(SHLIB_PREFIX)
LIB_SUFFIX = $(SHLIB_SUFFIX)
endif

define lib_rules
$(1)_LIBNAME = grass_$(2).$(GRASS_VERSION_NUMBER)
ifneq ($(NEED_DEPS),)
$(1)LIB = -l$$($(1)_LIBNAME) $$($(1)DEPS)
else
$(1)LIB = -l$$($(1)_LIBNAME)
endif
ifneq ($(1),IOSTREAM)
$(1)DEP = $$(BASE_LIBDIR)/$$(LIB_PREFIX)$$($(1)_LIBNAME)$$(LIB_SUFFIX)
else
$(1)DEP = $$(BASE_LIBDIR)/$$(STLIB_PREFIX)$$($(1)_LIBNAME)$$(STLIB_SUFFIX)
endif
endef

$(foreach lib,$(libs),$(eval $(call lib_rules,$(firstword $(subst :, ,$(lib))),$(lastword $(subst :, ,$(lib))))))

ifneq ($(MINGW),)
GISLIB += $(INTLLIB)
endif

DBMILIB     = $(DBMICLIENTLIB) $(DBMIBASELIB) $(DBMIEXTRALIB)
GEOMLIB     = $(OPTRILIB) $(SOSLIB) $(LIALIB) $(BASICLIB)

DBMIDEPS    = $(DBMICLIENTDEPS) $(DBMIBASEDEPS)
GEOMDEPS    = $(OPTRIDEPS) $(SOSDEPS) $(LIADEPS) $(BASICDEPS)
