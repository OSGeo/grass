MODULE_TOPDIR = ../..

PGM = v.digit

LIBES = $(DISPLAYLIB) $(RASTERLIB) $(VECTLIB) $(GISLIB) $(TCLTKLIBPATH) $(TCLTKLIBS)
DEPENDENCIES= $(DISPLAYDEP) $(RASTERDEP) $(VECTDEP) $(GISDEP)

EXTRA_CFLAGS = $(XCFLAGS) $(TCLINCDIR) $(TKINCDIR) $(VECT_CFLAGS)
XTRA_LDFLAGS=$(DFLAGS) 
EXTRA_INC = $(VECT_INC)

include $(MODULE_TOPDIR)/include/Make/Module.make

AUXDIR := $(GISBASE)/etc/v.digit
TCLFILES := $(patsubst %,$(AUXDIR)/%,$(wildcard *.tcl))
GIFFILES := $(patsubst icons/%,$(AUXDIR)/%,$(wildcard icons/*.gif))
XBMFILES := $(patsubst icons/%,$(AUXDIR)/%,$(wildcard icons/*.xbm))

default: cmd $(TCLFILES) $(GIFFILES) $(XBMFILES)

$(AUXDIR):
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	if [ ! -d $(AUXDIR) ]; then $(MKDIR) $@; fi

$(AUXDIR)/%.tcl: %.tcl $(AUXDIR)
	$(INSTALL_DATA) $< $@

$(AUXDIR)/%.gif: icons/%.gif $(AUXDIR)
	$(INSTALL_DATA) $< $@

$(AUXDIR)/%.xbm: icons/%.xbm $(AUXDIR)
	$(INSTALL_DATA) $< $@

