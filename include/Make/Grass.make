# to force make to use /bin/sh
SHELL           = /bin/sh

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
VECT_INC        = 

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

GRASS_VERSION_FILE    = $(ETC)/VERSION
GRASS_BUILD_FILE      = $(ETC)/BUILD

##################### other #############################################

COMPILE_FLAGS      = $(CPPFLAGS) $(CFLAGS1) $(INCLUDE_DIRS)
COMPILE_FLAGS_CXX  = $(CPPFLAGS) $(CXXFLAGS1) $(INCLUDE_DIRS)

# crude hack for vector LFS, LFS_FLAGS should be set by configure
ifdef USE_LARGEFILES
VECT_LFS_FLAGS = -D_FILE_OFFSET_BITS=64
endif

CFLAGS      =  $(INC) $(COMPILE_FLAGS)
CXXFLAGS    =  $(INC) $(COMPILE_FLAGS_CXX)
LDFLAGS     =  $(LIBPATH) $(LINK_FLAGS) $(LD_SEARCH_FLAGS) $(PQLIBPATH)
VECT_CFLAGS =  $(GDALCFLAGS) $(GEOSCFLAGS) $(VECT_LFS_FLAGS)

# Object with _fmode which must be linked to each executable on Windows
ifdef MINGW
FMODE_OBJ = $(BASE_LIBDIR)/fmode.o
endif

# lexical analyzer and default options
LFLAGS      = -s

# parser generator and default options
YFLAGS      = -d -v

MANSECT = 1
MANBASEDIR = $(ARCH_DISTDIR)/man
MANDIR = $(MANBASEDIR)/man$(MANSECT)
HTML2MAN = VERSION_NUMBER=$(GRASS_VERSION_NUMBER) $(GISBASE)/tools/g.html2man.py

GDAL_LINK = $(USE_GDAL)
GDAL_DYNAMIC = 1

##################### library switches ##################################

libs = \
	ARRAYSTATS:arraystats \
	BASIC:basic \
	BITMAP:bitmap \
	BTREE:btree \
	CAIRODRIVER:cairodriver \
	CDHC:cdhc \
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
	FORM:form \
	G3D:g3d \
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
	TRANS:trans \
	VECTOR:vector \
	VEDIT:vedit \
	XDISPLAY:xdisplay \
	XGD:xgd \
	XGI:xgi \
	XPM:xpm \
	IOSTREAM:iostream

ARRAYSTATSDEPS  = $(GISLIB)
BITMAPDEPS      = $(LINKMLIB)
CAIRODRIVERDEPS = $(DRIVERLIB) $(GISLIB) $(CAIROLIB) $(FCLIB)
CLUSTERDEPS     = $(IMAGERYLIB) $(GISLIB)
DBMIBASEDEPS    = $(GISLIB)
DBMICLIENTDEPS  = $(DBMIBASELIB) $(GISLIB)
DBMIDRIVERDEPS  = $(DBMIBASELIB) $(DBSTUBSLIB) $(GISLIB)
DBSTUBSDEPS     = $(DBMIBASELIB) $(GISLIB)
DIG2DEPS        = $(GISLIB) $(RTREELIB)
DISPLAYDEPS     = $(GISLIB) $(PNGDRIVERLIB) $(PSDRIVERLIB) $(HTMLDRIVERLIB) \
		$(DRIVERLIB) $(RASTERLIB)
DRIVERDEPS      = $(GISLIB) $(FTLIB) $(ICONVLIB)
FORMDEPS        = $(DBMILIB) $(GISLIB)
G3DDEPS         = $(RASTERLIB) $(GISLIB)
GISDEPS         = $(DATETIMELIB) $(PTHREADLIBPATH) $(PTHREADLIB) $(INTLLIB) \
		$(MATHLIB) $(ZLIBLIBPATH) $(ZLIB)
GMATHDEPS       = $(GISLIB) $(FFTWLIB) $(LAPACKLIB) $(BLASLIB)
GPDEDEPS        = $(GISLIB) $(G3DLIB)
GPROJDEPS       = $(GISLIB) $(PROJLIB) $(GDALLIBS)
HTMLDRIVERDEPS  = $(DRIVERLIB) $(GISLIB)
IMAGERYDEPS     = $(GISLIB)
INTERPFLDEPS    = $(BITMAPLIB) $(DBMILIB) $(GMATHLIB) $(INTERPDATALIB) \
		$(QTREELIB) $(VECTLIB) $(RASTERLIB) $(GISLIB)
IORTHODEPS      = $(IMAGERYLIB) $(GISLIB)
LIDARDEPS       = $(VECTLIB) $(DBMILIB) $(RASTERLIB) $(SEGMENTLIB) $(GMATHLIB) \
		$(GISLIB) $(MATHLIB)
LRSDEPS         = $(VECTLIB) $(DBMILIB) $(GISLIB)
MANAGEDEPS	= $(VECTLIB) $(G3DLIB) $(GISLIB)
NVIZDEPS        = $(BITMAPLIB) $(SITESLIB) $(VECTLIB) $(G3DLIB) $(OGSFLIB) \
		$(RASTERLIB) $(GISLIB) $(OPENGLLIB)
OGSFDEPS        = $(SITESLIB) $(G3DLIB) $(RASTERLIB) $(BITMAPLIB) $(VECTLIB) \
		$(GISLIB) $(XLIBPATH) $(OPENGLLIB) $(OPENGLULIB) $(TIFFLIBPATH) \
		$(TIFFLIB) $(FFMPEGLIBPATH) $(FFMPEGLIB)
PNGDRIVERDEPS   = $(DRIVERLIB) $(GISLIB) $(PNGLIB)
PSDRIVERDEPS    = $(DRIVERLIB) $(GISLIB)
RASTERDEPS      = $(GISLIB) $(XDRLIB) $(SOCKLIB)
RLIDEPS         = $(RASTERLIB) $(GISLIB)
SEGMENTDEPS     = $(GISLIB)
SIMDEPS         = $(BITMAPLIB) $(GMATHLIB) $(LINKMLIB) $(SITESLIB) $(VECTLIB) \
		$(DBMILIB) $(GISLIB)
SITESDEPS       = $(DBMILIB) $(VECTORLIB) $(GISLIB) $(DATETIMELIB) 
STATSDEPS       = $(GISLIB)
SYMBDEPS        = $(GISLIB)
VECTORDEPS      = $(DBMILIB) $(GRAPHLIB) $(DIG2LIB) $(GISLIB) $(LINKMLIB) \
		$(RTREELIB) $(GDALLIBS) $(GEOSLIBS)
VEDITDEPS       = $(GISLIB) $(VECTORLIB)

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
endif

define lib_rules
$(1)_LIBNAME = grass_$(2)
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

DBMILIB     = $(DBMICLIENTLIB) $(DBMIBASELIB) $(DBMIEXTRALIB)
GEOMLIB     = $(OPTRILIB) $(SOSLIB) $(LIALIB) $(BASICLIB)
VECTLIB     = $(VECTORLIB) $(DIG2LIB) $(GRAPHLIB) $(RTREELIB) $(LINKMLIB) $(DBMILIB)

DBMIDEP     = $(DBMICLIENTDEP) $(DBMIBASEDEP)
GEOMDEP     = $(OPTRIDEP) $(SOSDEP) $(LIADEP) $(BASICDEP)
VECTDEP     = $(VECTORDEP) $(DIG2DEP) $(GRAPHDEP) $(RTREEDEP) $(LINKMDEP) $(DBMIDEP)
