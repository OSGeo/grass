MODULE_TOPDIR = ../..

PGM=r3.out.v5d

#for Linux/x86 et al. (needs fix for MacOSX)
EXTRA_CFLAGS = -DLITTLE

#compile if LFS Large File Support present:
ifneq ($(USE_LARGEFILES),)
	EXTRA_CFLAGS += -D_FILE_OFFSET_BITS=64
endif

LIBES = $(G3DLIB) $(GISLIB)
DEPENDENCIES = $(G3DDEP) $(GISDEP)

include $(MODULE_TOPDIR)/include/Make/Module.make

default: cmd
