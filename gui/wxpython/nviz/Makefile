MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Lib.make

LIB_NAME = grass7_wxnviz
SOURCES := $(wildcard *.cpp) $(LIB_NAME)_wrap.cpp
SHLIB_OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

EXTRA_CFLAGS = $(SHLIB_CFLAGS) $(GDALCFLAGS) $(PYTHONCFLAGS) $(WXWIDGETSCXXFLAGS) $(XCFLAGS) $(XMINC)
EXTRA_LIBS = $(GISLIB) $(WXWIDGETSLIB) $(OGSFLIB) $(NVIZLIB) $(OPENGLLIB) $(OPENGLULIB)
ifeq ($(findstring darwin,$(ARCH)),darwin)
EXTRA_LIBS := -bundle -undefined dynamic_lookup $(EXTRA_LIBS)
else
EXTRA_LIBS := $(PYTHONLDFLAGS) $(EXTRA_LIBS)
endif

LOCAL_HEADERS = nviz.h

ETCDIR = $(ETC)/wxpython

SHLIB = $(OBJDIR)/_$(LIB_NAME).so

EXTRA_CLEAN_FILES = $(SHLIB) $(LIB_NAME).i $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

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

$(LIB_NAME).py $(LIB_NAME)_wrap.cpp: $(LIB_NAME).i
	$(SWIG) -c++ -python -shadow -o $(LIB_NAME)_wrap.cpp $<

$(SHLIB): $(SHLIB_OBJS)
ifeq ($(findstring darwin,$(ARCH)),darwin)
	$(CXX) -o $@ $(LDFLAGS) $^ $(EXTRA_LIBS)
else
	$(SHLIB_LD) -o $@ $(LDFLAGS) $^ $(EXTRA_LIBS)
endif

install_nviz: $(SHLIB) $(LIB_NAME).py
	$(INSTALL) $(SHLIB) $(ETCDIR)/nviz
	$(INSTALL_DATA) $(LIB_NAME).py $(ETCDIR)/nviz
