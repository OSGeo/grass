# $Id$
# Makefile for building nvwish
# Requires Tk 8.x and Tcl 8.x

MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Platform.make
include $(MODULE_TOPDIR)/include/Make/Grass.make
include $(MODULE_TOPDIR)/include/Make/Rules.make

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

# Stuff for mkdspf - not yet implemented
# DSPLIB = $(LIBDIR)/libes/dspf/libdsp3.a
# DSPINC = -I$(LIBDIR)/libes/dspf

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS= $(TCLINCDIR) $(TKINCDIR) \
	$(INC) \
	$(TIFFINCPATH) $(DSPINC) \
	$(VECT_CFLAGS) $(OPENGLINC)

ifeq ($(OPENGL_X11),1)
EXTRA_CFLAGS += $(XCFLAGS)
endif

XTRA_LDFLAGS=$(TCLTKLIBPATH) $(TCLTKLIBS) $(OGL_LIBS)

OBJS = nvizAppInit.o change_view.o draw.o exag.o glwrappers.o \
	init_commands.o lights.o map_obj.o misc.o nviz_init.o \
	position.o quick_draw.o anim_support.o cutplane_obj.o \
	script_support.o do_zoom.o \
	label.o nvizMain.o togl.o togl_cb.o\
	query_vect.o \
	volume.o \
	togl_flythrough.o pick_vect_commands.o site_attr_commands.o site_highlight_commands.o

# mkdspf_calc_linefax.o mkdspf_init_file.o mkdspf_main.o \
# mkdspf_fill_cfax.o mkdspf_iso_surface.o

ARCH_OBJS := $(foreach obj,$(OBJS),$(OBJDIR)/$(obj)) 

EXTRA_CLEAN_FILES = nvwish$(EXE)

#default: $(BIN)/$(PGM) htmlcmd
default: nvwish$(EXE)

nvwish$(EXE): $(ARCH_OBJS) $(VECTDEP) $(BITMAPDEP) $(LINKMDEP) $(FORMDEP) $(DBMIDEP $(GISDEP)
	$(CC) $(LDFLAGS) -o $@ $(ARCH_OBJS) $(FMODE_OBJ) $(SURFLIB) \
		$(BITMAPLIB) $(LINKMLIB) $(VECTLIB) $(SITESLIB) $(FORMLIB) $(DBMILIB) \
		$(G3DLIB) $(GISLIB) $(XDRLIB) \
		$(DATETIMELIB) $(XTRA_LDFLAGS) $(TIFFLIBPATH) $(TIFFLIB) \
		$(MATHLIB)
	-$(MKDIR) $(GISBASE)/docs/html
	-$(INSTALL_DATA) description.html $(GISBASE)/docs/html/nviz.html
	-$(INSTALL) *.png *.jpg $(GISBASE)/docs/html 2> /dev/null ; true

