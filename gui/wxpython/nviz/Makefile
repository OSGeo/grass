MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Lib.make

LIB_NAME = grass7_wxnviz

SHLIB = $(OBJDIR)/_$(LIB_NAME).so

ETCDIR = $(ETC)/wxpython

ifneq ($(USE_WXWIDGETS),)
ifneq ($(USE_PYTHON),)
ifneq ($(strip $(CXX)),)
ifneq ($(strip $(OPENGLLIB)),)
default: install_nviz
endif
endif
endif
endif

$(LIB_NAME).i: nviz.i nviz_types.i nviz.h
	cat nviz.i nviz_types.i > $(LIB_NAME).i
	echo "/* auto-generated swig typedef file */" >> $(LIB_NAME).i
	cat nviz.h >> $(LIB_NAME).i

$(LIB_NAME).py: $(SHLIB)

$(SHLIB): $(LIB_NAME).i
	python setup.py build_ext --swig=$(SWIG) --build-lib=$(OBJDIR) --build-temp=$(OBJDIR)

.NOTPARALLEL: $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

install_nviz: $(ETCDIR)/nviz/_$(LIB_NAME).so $(ETCDIR)/nviz/$(LIB_NAME).py

$(ETCDIR)/nviz/_$(LIB_NAME).so: $(SHLIB)
	$(INSTALL) $< $@

$(ETCDIR)/nviz/$(LIB_NAME).py: $(LIB_NAME).py
	$(INSTALL_DATA) $< $@

