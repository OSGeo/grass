MODULE_TOPDIR = ../..

include $(MODULE_TOPDIR)/include/Make/Other.make

MDFILES := \
	github_guide.md \
	investigating_history.md \
	style_guide.md

default: $(patsubst %,$(MDDIR)/source/%,$(MDFILES))
