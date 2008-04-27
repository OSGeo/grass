MODULE_TOPDIR = ../../..

EXTRA_LIBS=$(GISLIB) $(VECTLIB) $(G3DLIB)

LIB_NAME = $(MANAGE_LIBNAME)

LOCAL_HEADERS = $(wildcard *.h) ../list.h

EXTRA_INC = -I.. $(VECT_INC)
EXTRA_CFLAGS = $(VECT_CFLAGS)
DEPENDENCIES = $(GISDEP) $(VECTDEP) $(G3DDEP)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: stlib $(ETC)/element_list

$(ETC)/element_list: Element_List
	if [ ! -d $(ETC) ]; then $(MKDIR) $(ETC); fi
	$(INSTALL_DATA) $? $@
