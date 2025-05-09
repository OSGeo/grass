MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Other.make

FILES := $(wildcard *.html)
MDFILES := $(wildcard *.md)

default: $(patsubst %,$(HTMLDIR)/%,$(FILES)) $(patsubst %,$(MDDIR)/source/%,$(MDFILES))
