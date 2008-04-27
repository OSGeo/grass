MODULE_TOPDIR = ../..

EXTRA_LIBS=$(GISLIB)

LIB_NAME = $(SYMB_LIBNAME)

include $(MODULE_TOPDIR)/include/Make/Lib.make

default: lib symb

symb:
	if [ ! -d $(ETC)/symbol ]; then $(MKDIR) $(ETC)/symbol ; fi
	if [ ! -d $(ETC)/symbol/demo ]; then $(MKDIR) $(ETC)/symbol/demo ; fi
	if [ ! -d $(ETC)/symbol/basic ]; then $(MKDIR) $(ETC)/symbol/basic ; fi
	if [ ! -d $(ETC)/symbol/extra ]; then $(MKDIR) $(ETC)/symbol/extra ; fi
	@# check for file type to avoid problems with CVS/ directory:
	for file in symbol/demo/* ; do \
		[ -f "$$file" ] && $(INSTALL_DATA) $$file $(ETC)/symbol/demo ; \
	done
	for file in symbol/basic/* ; do \
		[ -f "$$file" ] && $(INSTALL_DATA) $$file $(ETC)/symbol/basic ; \
	done
	for file in symbol/extra/* ; do \
		[ -f "$$file" ] && $(INSTALL_DATA) $$file $(ETC)/symbol/extra ; \
	done
