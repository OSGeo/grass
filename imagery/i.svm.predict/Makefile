MODULE_TOPDIR = ../..

PGM = i.svm.predict

LIBES = $(RASTERLIB) $(IMAGERYLIB) $(GISLIB) $(LIBSVM_LIB)
DEPENDENCIES = $(RASTERDEP) $(IMAGERYDEP) $(GISDEP)

EXTRA_INC = $(LIBSVM_INC)

include $(MODULE_TOPDIR)/include/Make/Module.make

ifneq ($(USE_LIBSVM),)
default: cmd
endif
