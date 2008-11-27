MODULE_TOPDIR = ../..

PGM = db.test

include $(MODULE_TOPDIR)/include/Make/Script.make

TEST_SRC := $(wildcard test*)
TEST_DST := $(patsubst %,$(ETC)/db.test/%,$(TEST_SRC))

default: script tests

tests:
	if [ ! -d $(ETC)/db.test ]; then $(MKDIR) $(ETC)/db.test; fi
	$(MAKE) $(TEST_DST)

$(ETC)/db.test/%: %
	$(INSTALL_DATA) $< $@
