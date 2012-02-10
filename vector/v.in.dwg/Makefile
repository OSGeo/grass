MODULE_TOPDIR = ../..

PGM=v.in.dwg
ADINIT = $(ETC)/adinit.dat
ADINITSRC := $(shell echo $(OPENDWGINCPATH) | cut -b3- )/adinit/adinit.dat

DEPENDENCIES = $(VECTORDEP) $(DBMIDEP) $(GISDEP)
LIBES     = $(VECTORLIB) $(DBMILIB) $(GISLIB) $(OPENDWGLIBPATH) $(OPENDWGLIB)
EXTRA_INC = $(VECT_INC) $(OPENDWGINCPATH)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_OPENDWG),)
default: check
	$(MAKE) cmd $(ADINIT)
endif

check:
ifneq ($(strip $(MINGW)),)
	cat WARNING
else
	cat WARNING >/dev/tty
endif
	@read IN </dev/tty ; if [ "$$IN" != "yes" ]; then exit 1; fi

.PHONY: check

#a bit ugly:
$(ADINIT): $(ADINITSRC)
	$(INSTALL_DATA) $< $@
