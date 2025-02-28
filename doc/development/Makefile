MODULE_TOPDIR = ..

include $(MODULE_TOPDIR)/include/Make/Other.make

# So far, we disntinguished user and contributor documentation here by
# extension. This is no longer possible with Markdown.
MDFILES := github_guide.md style_guide.md

default: $(patsubst %,$(MDDIR)/source/%,$(MDFILES))
