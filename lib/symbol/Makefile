MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB)

LIB_NAME = $(SYMB_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

SYMBOL_SRC := $(wildcard symbol/*/*)
SYMBOL_DST := $(patsubst symbol/%,$(ETC)/symbol/%,$(SYMBOL_SRC))

default: lib
	$(MAKE) dirs
	$(MAKE) symb

dirs:
	if [ ! -d $(ETC)/symbol ]; then $(MKDIR) $(ETC)/symbol ; fi
	if [ ! -d $(ETC)/symbol/demo ]; then $(MKDIR) $(ETC)/symbol/demo ; fi
	if [ ! -d $(ETC)/symbol/basic ]; then $(MKDIR) $(ETC)/symbol/basic ; fi
	if [ ! -d $(ETC)/symbol/extra ]; then $(MKDIR) $(ETC)/symbol/extra ; fi

symb:	$(SYMBOL_DST)

$(ETC)/symbol/%: symbol/%
	$(INSTALL_DATA) $< $@

.PHONY: dirs symb
