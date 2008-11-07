MODULE_TOPDIR = ../..

PGM = g.mlist

EXTRA_INC = $(REGEXINCPATH)
LIBES = $(MANAGELIB) $(GISLIB) $(REGEXLIBPATH) $(REGEXLIB)
DEPENDENCIES = $(MANAGEDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_REGEX),)
default: cmd
else
default:
endif
