# Makefile for building nvwish
# Requires Tk 8.x and Tcl 8.x

MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Vars.make
MOD_OBJS := $(filter-out mkdspf_%.o,$(AUTO_OBJS))
include $(MODULE_TOPDIR)/include/Make/Other.make
include $(MODULE_TOPDIR)/include/Make/Compile.make

# These should be set by the doconfigure script
NVWISH_VER=NVWISH2.2

ifeq ($(OPENGL_X11),1)
OGL_LIBS := $(OPENGLULIB) $(OPENGLLIB) $(XLIBPATH) $(XLIB) -lXmu -lXext $(XEXTRALIBS) $(MATHLIB)
endif
ifeq ($(OPENGL_WINDOWS),1)
OGL_LIBS := $(OPENGLULIB) $(OPENGLLIB) -lgdi32 $(MATHLIB)
endif
ifeq ($(OPENGL_AQUA),1)
OGL_LIBS := $(OPENGLULIB) $(OPENGLLIB) $(MATHLIB)
endif

SURFLIB = $(OGSFLIB)

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS= $(TCLINCDIR) $(TKINCDIR) \
	$(INC) \
	$(TIFFINCPATH) $(DSPINC) \
	$(VECT_CFLAGS) $(OPENGLINC)

ifeq ($(OPENGL_X11),1)
EXTRA_CFLAGS += $(XCFLAGS)
endif

EXTRA_LDFLAGS=$(TCLTKLIBPATH) $(TCLTKLIBS) $(OGL_LIBS)

LIBES = $(SURFLIB) \
	$(BITMAPLIB) $(LINKMLIB) $(VECTLIB) $(SITESLIB) $(FORMLIB) $(DBMILIB) \
	$(G3DLIB) $(GISLIB) \
	$(DATETIMELIB) $(TIFFLIBPATH) $(TIFFLIB)

DEPENDENCIES = $(VECTDEP) $(BITMAPDEP) $(LINKMDEP) $(FORMDEP) $(DBMIDEP) $(GISDEP)

EXTRA_CLEAN_FILES = nvwish$(EXE)

default: nvwish$(EXE)

nvwish$(EXE): $(ARCH_OBJS) $(DEPENDENCIES)
	$(call linker)
