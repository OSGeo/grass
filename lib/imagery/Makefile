MODULE_TOPDIR = ../..

LIB = IMAGERY

EXTRA_INC = $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make
include $(MODULE_TOPDIR)/include/Make/Grass.make

# images
IMGSRC := $(wildcard *.png) $(wildcard *.jpg) $(wildcard *.gif)
IMGDST := $(patsubst %,$(HTMLDIR)/%,$(IMGSRC))

default: $(IMGDST) lib

#doxygen:
DOXNAME=imagery
