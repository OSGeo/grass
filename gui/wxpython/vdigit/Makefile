MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Lib.make

LIB_NAME = grass7_wxvdigit

SHLIB = $(OBJDIR)/_$(LIB_NAME).so

ETCDIR = $(ETC)/wxpython

ifneq ($(USE_WXWIDGETS),)
ifneq ($(USE_PYTHON),)
ifneq ($(strip $(CXX)),)
default: install_vdigit
endif
endif
endif

$(LIB_NAME).i: digit.i dig_types.i pseudodc.i driver.h digit.h
	cat digit.i pseudodc.i dig_types.i > $(LIB_NAME).i
	echo "/* auto-generated swig typedef file */" >> $(LIB_NAME).i
	cat driver.h digit.h >> $(LIB_NAME).i

$(LIB_NAME).py: $(SHLIB)

$(SHLIB): $(LIB_NAME).i
	python setup.py build_ext --swig=$(SWIG) --swig-opts=-c++ --build-lib=$(OBJDIR) --build-temp=$(OBJDIR)

.NOTPARALLEL: $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

install_vdigit: $(ETCDIR)/vdigit/_$(LIB_NAME).so $(ETCDIR)/vdigit/$(LIB_NAME).py

$(ETCDIR)/vdigit/_$(LIB_NAME).so: $(SHLIB)
	$(INSTALL) $< $@

$(ETCDIR)/vdigit/$(LIB_NAME).py: $(LIB_NAME).py
	$(INSTALL_DATA) $< $@
