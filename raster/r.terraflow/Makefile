MODULE_TOPDIR = ../..

PGM = r.terraflow

include $(MODULE_TOPDIR)/include/Make/Multi.make

SOURCES = main.cc common.cc stats.cc fill.cc types.cc ccforest.cc \
	nodata.cc plateau.cc direction.cc water.cc  \
	filldepr.cc grid.cc genericWindow.cc \
	flow.cc sweep.cc weightWindow.cc

OBJARCH=OBJ.$(ARCH)

FLOAT_OBJ := $(patsubst %.cc, $(OBJARCH)/FLOAT/%.o, $(SOURCES))
SHORT_OBJ := $(patsubst %.cc, $(OBJARCH)/SHORT/%.o, $(SOURCES))

IOSTREAM_LIBNAME = iostream
IOSTREAM_DIR = IOStream
IOSTREAM_INC = $(IOSTREAM_DIR)/include
IOSTREAM_LIB =  -l$(IOSTREAM_LIBNAME)
IOSTREAM_DEPLIB = $(ARCH_LIBDIR)/$(STLIB_PREFIX)$(IOSTREAM_LIBNAME)$(STLIB_SUFFIX)

CXXFLAGS += -I$(IOSTREAM_INC) \
		-DUSER=\"$(USER)\" \
		-DNODATA_FIX -D_FILE_OFFSET_BITS=64

LIBS = $(GISLIB) 
DEPLIBS = $(DEPGISLIB)

CLEAN_SUBDIRS = $(IOSTREAM_DIR)

#Note: 	if a header file is modified, the .o files do not get rebuilt..
#		header files should be included as prerequisites, but does not work 
#		because of GRASS scripts 
ifneq ($(BROKEN_MAKE),)
$(OBJARCH)/FLOAT/%.o: %.cc $(LOCAL_HEADERS) $(OBJARCH)/FLOAT
else
$(OBJARCH)/FLOAT/%.o: %.cc $(LOCAL_HEADERS) | $(OBJARCH)/FLOAT
endif
	$(CXX) -c $(CXXFLAGS) $(NLS_CFLAGS) -DELEV_FLOAT $< -o $@

ifneq ($(BROKEN_MAKE),)
$(OBJARCH)/SHORT/%.o: %.cc $(LOCAL_HEADERS) $(OBJARCH)/SHORT
else
$(OBJARCH)/SHORT/%.o: %.cc $(LOCAL_HEADERS) | $(OBJARCH)/SHORT
endif
	$(CXX) -c $(CXXFLAGS) $(NLS_CFLAGS) -DELEV_SHORT $< -o $@

default: $(BIN)/$(PGM)$(EXE) $(BIN)/$(PGM).short$(EXE)
	$(MAKE) htmlcmd

$(OBJARCH)/FLOAT:
	$(MKDIR) $@

$(OBJARCH)/SHORT:
	$(MKDIR) $@

$(BIN)/$(PGM)$(EXE): $(FLOAT_OBJ) $(DEPLIBS) $(IOSTREAM_DEPLIB)
	$(CXX) -DELEV_FLOAT $(LDFLAGS) -o $@ $(FLOAT_OBJ) $(LIBS) $(MATHLIB) \
	$(XDRLIB) $(IOSTREAM_LIB)

$(BIN)/$(PGM).short$(EXE): $(SHORT_OBJ) $(DEPLIBS) $(IOSTREAM_DEPLIB)
	$(CXX) -DELEV_SHORT $(LDFLAGS) -o $@ $(SHORT_OBJ) $(LIBS) $(MATHLIB) \
	$(XDRLIB) $(IOSTREAM_LIB)

$(IOSTREAM_DEPLIB):
	$(MAKE) -C $(IOSTREAM_DIR)

