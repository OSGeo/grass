MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

MDFILES := \
	github_guide.md \
	investigating_history.md \
	style_guide.md \
	building_with_cmake.md

default: $(patsubst %,$(MDDIR)/source/%,$(MDFILES))
