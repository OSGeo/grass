MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Lib.make

# substitute OSX arch flags for wxpython
ifneq ($(MACOSX_ARCHS),)
CFLAGS := $(subst $(MACOSX_ARCHS),,$(CFLAGS)) $(MACOSX_ARCHS_WXPYTHON)
CXXFLAGS := $(subst $(MACOSX_ARCHS),,$(CXXFLAGS)) $(MACOSX_ARCHS_WXPYTHON)
LDFLAGS := $(subst $(MACOSX_ARCHS),,$(LDFLAGS)) $(MACOSX_ARCHS_WXPYTHON)
endif

LIB_NAME = grass7_wxnviz

SHLIB = $(OBJDIR)/_$(LIB_NAME).so

ETCDIR = $(ETC)/wxpython

EXTRA_CLEAN_FILES = $(LIB_NAME).i $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

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
	GISBASE="$(GISBASE)" \
	ARCH_DISTDIR="$(ARCH_DISTDIR)" \
	GDALCFLAGS="$(GDALCFLAGS)" \
	GDALLIBS="$(GDALLIBS)" \
	WXWIDGETSCXXFLAGS="$(WXWIDGETSCXXFLAGS)" \
	OPENGLINC="$(OPENGLINC)" \
	OPENGLLIB="$(OPENGLLIB)" \
	OPENGLULIB="$(OPENGLULIB)" \
	WXWIDGETSLIB="$(WXWIDGETSLIB)" \
	OPENGL_X11="$(OPENGL_X11)" \
	XCFLAGS="$(XCFLAGS)" \
	$(PYTHON) setup.py build_ext --swig=$(SWIG) --swig-opts=-c++ --build-lib=$(OBJDIR) --build-temp=$(OBJDIR)

.NOTPARALLEL: $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

install_nviz: $(ETCDIR)/nviz/_$(LIB_NAME).so $(ETCDIR)/nviz/$(LIB_NAME).py

$(ETCDIR)/nviz/_$(LIB_NAME).so: $(SHLIB)
	$(INSTALL) $< $@

$(ETCDIR)/nviz/$(LIB_NAME).py: $(LIB_NAME).py
	$(INSTALL_DATA) $< $@

