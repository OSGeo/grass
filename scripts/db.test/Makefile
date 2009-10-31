MODULE_TOPDIR = ../..

PGM = db.test

include $(MODULE_TOPDIR)/include/Make/Script.make

TEST_SRC := $(wildcard test*)
TEST_DST := $(patsubst %,$(ETC)/db.test/%,$(TEST_SRC))

default: script $(TEST_DST)

$(ETC)/db.test/%: % | $(ETC)/db.test
	$(INSTALL_DATA) $< $@

$(ETC)/db.test:
	$(MKDIR) $@
