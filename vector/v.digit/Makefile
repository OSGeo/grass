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
GIFFILES := $(patsubst icons/%,$(AUXDIR)/%,$(wildcard icons/*.gif))
XBMFILES := $(patsubst icons/%,$(AUXDIR)/%,$(wildcard icons/*.xbm))

pre: $(AUXDIR)

default: cmd $(TCLFILES) $(GIFFILES) $(XBMFILES)

$(AUXDIR):
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	if [ ! -d $(AUXDIR) ]; then $(MKDIR) $@; fi

$(AUXDIR)/%.tcl: %.tcl
	$(INSTALL_DATA) $< $@

$(AUXDIR)/%.gif: icons/%.gif
	$(INSTALL_DATA) $< $@

$(AUXDIR)/%.xbm: icons/%.xbm
	$(INSTALL_DATA) $< $@

