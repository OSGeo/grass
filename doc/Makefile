MODULE_TOPDIR = ..

SUBDIRS = \
	development

include $(MODULE_TOPDIR)/include/Make/Dir.make

FILES := $(wildcard *.html)
# So far, we disntinguished user and contributor documentation here by
# extension. This is no longer possible with Markdown.
MDFILES := grass_database.md \
	projectionintro.md \
	jupyter_intro.md \
	interfaces_overview.md

default: $(patsubst %,$(HTMLDIR)/%,$(FILES)) $(patsubst %,$(MDDIR)/source/%,$(MDFILES)) parsubdirs
