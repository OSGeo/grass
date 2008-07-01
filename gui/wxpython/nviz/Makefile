MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Lib.make

LIB_NAME = grass6_wxnviz
SOURCES := $(wildcard *.cpp) $(LIB_NAME)_wrap.cpp
SHLIB_OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

EXTRA_CFLAGS = $(SHLIB_CFLAGS) $(GDALCFLAGS) $(PYTHONCFLAGS) $(WXWIDGETSCXXFLAGS) $(XCFLAGS) $(XMINC)
EXTRA_LIBS = $(GISLIB) $(WXWIDGETSLIB) $(PYTHONLDFLAGS) $(XLIBPATH) $(XMLIB) $(XTLIB) $(XLIB) $(XEXTRALIBS) $(OGSFLIB) $(NVIZLIB)

LOCAL_HEADERS = nviz.h

ETCDIR = $(ETC)/wxpython

SHLIB = $(OBJDIR)/_$(LIB_NAME).so

EXTRA_CLEAN_FILES = $(SHLIB) $(LIB_NAME).i $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

default: install_nviz

$(LIB_NAME).i: nviz.i dig_types.i nviz.h
	cat nviz.i dig_types.i > $(LIB_NAME).i
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
