MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

SRCFILES = WIND DEFAULT_WIND VAR PROJ_INFO PROJ_UNITS MYNAME
DBFMAPS = mysites point
SQLITEMAPS = country_boundaries
MAPFILES = dbln coor topo head cidx sidx hist
DBFFILES := $(foreach map,$(DBFMAPS),dbf/$(map).dbf)
SQLITEFILES = sqlite/sqlite.db
VECTFILES := $(foreach map,$(DBFMAPS),$(foreach file,$(MAPFILES),vector/$(map)/$(file))) $(foreach map,$(SQLITEMAPS),$(foreach file,$(MAPFILES),vector/$(map)/$(file)))
SRC := $(SRCFILES) $(DBFFILES) $(SQLITEFILES) $(VECTFILES)
SRCDIRS := PERMANENT PERMANENT/vector PERMANENT/dbf PERMANENT/sqlite $(patsubst %,PERMANENT/vector/%,$(DBFMAPS)) $(patsubst %,PERMANENT/vector/%,$(SQLITEMAPS))

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
