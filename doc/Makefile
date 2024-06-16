MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

FILES := $(wildcard *.html)

default: $(patsubst %,$(HTMLDIR)/%,$(FILES))
	 $(INSTALL_DATA) mkdocs.yml $(MDDIR)/
