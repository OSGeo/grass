MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

SRCFILES = WIND DEFAULT_WIND VAR PROJ_INFO PROJ_UNITS MYNAME
MAPS = mysites point
MAPFILES = dbln coor topo head cidx hist
DBFFILES := $(foreach map,$(MAPS),dbf/$(map).dbf)
VECTFILES := $(foreach map,$(MAPS),$(foreach file,$(MAPFILES),vector/$(map)/$(file)))
SRC := $(SRCFILES) $(DBFFILES) $(VECTFILES)
SRCDIRS := PERMANENT PERMANENT/vector PERMANENT/dbf $(patsubst %,PERMANENT/vector/%,$(MAPS))

DSTDIR = $(ARCH_DISTDIR)/demolocation
DST := $(patsubst %,$(DSTDIR)/PERMANENT/%,$(SRC))
DSTDIRS := $(patsubst %,$(DSTDIR)/%,$(SRCDIRS))
RCFILE = $(DSTDIR)/.grassrc$(GRASS_VERSION_MAJOR)$(GRASS_VERSION_MINOR)

default: $(DST) $(RCFILE)

$(DSTDIR)/PERMANENT/%: PERMANENT/% | $(DSTDIRS)
	$(INSTALL_DATA) $< $@

$(DSTDIRS): %:
	$(MKDIR) $@

$(RCFILE): grassrc.tmpl | $(DSTDIRS)
	sed 's!@GISDBASE@!$(RUN_GISBASE)!' < $< > $@
