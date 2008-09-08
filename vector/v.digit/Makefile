MODULE_TOPDIR = ../..

PGM = v.digit

LIBES = $(DISPLAYLIB) $(RASTERLIB) $(VECTLIB) $(GISLIB) $(TCLTKLIBPATH) $(TCLTKLIBS)
DEPENDENCIES= $(DISPLAYDEP) $(RASTERDEP) $(VECTDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

EXTRA_CFLAGS = $(XCFLAGS) $(TCLINCDIR) $(TKINCDIR) $(VECT_CFLAGS)
EXTRA_LDFLAGS = $(DFLAGS) 
EXTRA_INC = $(VECT_INC)

AUXDIR := $(GISBASE)/etc/v.digit
TCLFILES := $(patsubst %,$(AUXDIR)/%,$(wildcard *.tcl))

pre: $(AUXDIR)

default: cmd $(TCLFILES)

$(AUXDIR):
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	if [ ! -d $(AUXDIR) ]; then $(MKDIR) $@; fi

$(AUXDIR)/%.tcl: %.tcl
	$(INSTALL_DATA) $< $@

