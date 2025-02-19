MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

FILES := $(wildcard *.html)
# So far, we disntinguished user and contributor documentation here by
# extension. This is no longer possible with Markdown.
MDFILES := grass_database.md projectionintro.md

default: $(patsubst %,$(HTMLDIR)/%,$(FILES)) $(patsubst %,$(MDDIR)/source/%,$(MDFILES))
