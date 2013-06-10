MODULE_TOPDIR = ../..


LIB = SYMB

include $(MODULE_TOPDIR)/include/Make/Lib.make

SYMBOL_SRC := $(wildcard symbol/*/*)
SYMBOL_DST := $(patsubst symbol/%,$(ETC)/symbol/%,$(SYMBOL_SRC))
SYMBOL_DIRS := $(patsubst %,$(ETC)/symbol/%, demo basic extra geology n_arrows)

default: lib
	$(MAKE) $(SYMBOL_DST)

$(SYMBOL_DIRS): %: | $(ETC)/symbol
	$(MKDIR) $@

$(ETC)/symbol:
	$(MKDIR) $@

$(ETC)/symbol/%: symbol/% | $(SYMBOL_DIRS)
	$(INSTALL_DATA) $< $@
