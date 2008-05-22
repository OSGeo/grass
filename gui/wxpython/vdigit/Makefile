MODULE_TOPDIR = ../../..

include $(MODULE_TOPDIR)/include/Make/Lib.make

LIB_NAME = grass6_wxvdigit
SOURCES := $(wildcard *.cpp) $(LIB_NAME)_wrap.cpp
SHLIB_OBJS := $(patsubst %.cpp, $(OBJDIR)/%.o, $(SOURCES))

EXTRA_CFLAGS = $(SHLIB_CFLAGS) $(GDALCFLAGS) $(PYTHONCFLAGS) $(WXWIDGETSCXXFLAGS)
EXTRA_LIBS = $(VECTLIB) $(GISLIB) $(GDALLIBS) $(VEDITLIB) $(WXWIDGETSLIB) $(PYTHONLDFLAGS) -lgdi

LOCAL_HEADERS = digit.h driver.h

ETCDIR = $(ETC)/wxpython

SHLIB = $(OBJDIR)/_$(LIB_NAME)$(SHLIB_SUFFIX)

EXTRA_CLEAN_FILES = $(SHLIB) $(LIB_NAME).i $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

default: install_vdigit

$(LIB_NAME).i: digit.i dig_types.i driver.h digit.h
	cat digit.i dig_types.i > $(LIB_NAME).i
	echo "/* auto-generated swig typedef file */" >> $(LIB_NAME).i
	cat driver.h digit.h >> $(LIB_NAME).i

$(LIB_NAME).py $(LIB_NAME)_wrap.cpp: $(LIB_NAME).i
	$(SWIG) -c++ -python -shadow -o $(LIB_NAME)_wrap.cpp $<

.NOTPARALLEL: $(LIB_NAME).py $(LIB_NAME)_wrap.cpp

$(SHLIB): $(SHLIB_OBJS)
	$(SHLIB_LD) -o $@ $(LDFLAGS) $^ $(EXTRA_LIBS)

install_vdigit: $(SHLIB) $(LIB_NAME).py
	$(INSTALL) $(SHLIB) $(ETCDIR)/vdigit/
	$(INSTALL_DATA) $(LIB_NAME).py $(ETCDIR)/vdigit/
