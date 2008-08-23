MODULE_TOPDIR = ../../..

LIB_NAME = $(MANAGE_LIBNAME)
DEPENDENCIES = $(GISDEP) $(VECTDEP) $(G3DDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

LOCAL_HEADERS := $(wildcard *.h) ../list.h

EXTRA_INC = -I.. $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
EXTRA_LIBS = $(GISLIB) $(VECTLIB) $(G3DLIB)

default: stlib $(ETC)/element_list

$(ETC)/element_list: Element_List
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	$(INSTALL_DATA) $? $@
