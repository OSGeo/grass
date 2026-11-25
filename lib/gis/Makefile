MODULE_TOPDIR = ../..

LIB = GIS

LIBES = $(OPENMP_LIBPATH) $(OPENMP_LIB)
EXTRA_INC = $(ZLIBINCPATH) $(BZIP2INCPATH) $(ZSTDINCPATH) $(PTHREADINCPATH) $(REGEXINCPATH) $(OPENMP_INCPATH) $(PROJINC)
EXTRA_CFLAGS = $(OPENMP_CFLAGS) -DGRASS_VERSION_DATE=\"'$(GRASS_VERSION_DATE)'\"
CONFIG_DIR =$(ARCH_LIBDIR)/grass

PROJSRC = ellipse.table ellipse.table.solar.system datum.table \
	datumtransform.table FIPS.code state27 state83 projections

include $(MODULE_TOPDIR)/include/Make/Vars.make

MOD_OBJS := $(filter-out fmode.o,$(AUTO_OBJS))

include $(MODULE_TOPDIR)/include/Make/Lib.make
include $(MODULE_TOPDIR)/include/Make/Doxygen.make

PROJFILES := $(patsubst %,$(ETC)/proj/%,$(PROJSRC))

COLORSRC := $(filter-out colors/CVS,$(wildcard colors/[abcdefghijklmnopqrstuvwxyz]*))
COLORFILES := $(patsubst colors/%,$(ETC)/colors/%,$(COLORSRC))

default: lib $(FMODE_OBJ) $(PROJFILES) $(COLORFILES) $(ETC)/colors.desc $(ETC)/renamed_options $(CONFIG_DIR)/grass.conf

ifdef MINGW
$(FMODE_OBJ): $(OBJDIR)/fmode.o
	$(INSTALL) $< $@
endif

$(ETC)/colors:
	$(MKDIR) $@

$(ETC)/colors/%: colors/% | $(ETC)/colors
	$(INSTALL_DATA) $< $@

$(ETC)/proj:
	$(MKDIR) $@

$(ETC)/proj/%: % | $(ETC)/proj
	$(INSTALL_DATA) $< $@

$(ETC)/%: %
	$(INSTALL_DATA) $< $@

DOXNAME = gis

$(OBJDIR)/area_ellipse.o: pi.h
$(OBJDIR)/area_sphere.o: pi.h
$(OBJDIR)/geodesic.o: pi.h
$(OBJDIR)/geodist.o: pi.h
$(OBJDIR)/get_window.o: G.h
$(OBJDIR)/gisinit.o: G.h
$(OBJDIR)/radii.o: pi.h
$(OBJDIR)/rhumbline.o: pi.h
$(OBJDIR)/set_window.o: G.h

$(CONFIG_DIR):
	$(MKDIR) $@

$(CONFIG_DIR)/%: % | $(CONFIG_DIR)
	$(INSTALL_DATA) $< $@

$(CONFIG_DIR)/grass.conf: grass.conf | $(CONFIG_DIR)
	rm -f $@
	sed \
	-e 's#@CONFIG_PROJSHARE@#$(PROJSHARE)#' \
	-e 's#@GISBASE_INSTALL_PATH@##' \
	-e 's#@GRASS_INSTALL_CMAKECONFDIR@##' \
	-e 's#@GRASS_INSTALL_CMAKEMODULEDIR@##' \
	-e 's#@CMAKE_PREFIX_PATH@##' \
	-e 's#@CMAKE_C_COMPILER@##' \
	-e 's#@CMAKE_CXX_COMPILER@##' \
	-e 's#@GRASS_PREFIX_RELATIVE@#$(GRASS_PREFIX_RELATIVE)#' \
	-e 's#@GRASS_VERSION_GIT@#$(GRASS_VERSION_GIT)#' \
	-e 's#@GRASS_VERSION_MAJOR@#$(GRASS_VERSION_MAJOR)#' \
	-e 's#@GRASS_VERSION_MINOR@#$(GRASS_VERSION_MINOR)#' \
	-e 's#@GRASS_VERSION_NUMBER@#$(GRASS_VERSION_NUMBER)#' \
	-e 's#@GRASS_HEADERS_GIT_HASH@#$(GRASS_HEADERS_GIT_HASH)#' \
	-e 's#@GRASS_HEADERS_GIT_DATE@#$(GRASS_HEADERS_GIT_DATE)#' \
	-e 's#@BUILD_ARCH@#$(ARCH)#' \
	-e 's#@LD_LIBRARY_PATH_VAR@#$(LD_LIBRARY_PATH_VAR)#' \
	-e 's#@START_UP@#$(GRASS_NAME)#' \
	-e 's#@GRASS_COLORSDIR@#etc/colors#' \
	-e 's#@GRASS_DOCDIR@#docs/html#' \
	-e 's#@GRASS_ETCBINDIR@#etc#' \
	-e 's#@GRASS_ETCDIR@#etc#' \
	-e 's#@GRASS_FONTSDIR@#fonts#' \
	-e 's#@GRASS_GRAPHICSDIR@#docs/html#' \
	-e 's#@GRASS_GUISCRIPTDIR@#gui/scripts#' \
	-e 's#@GRASS_LOCALEDIR@#locale#' \
	-e 's#@GRASS_MISCDIR@##' \
	-e 's#@GRASS_MKDOCSDIR@#docs/mkdocs#' \
	$< > $@

grass.conf:
	cp resource_dirs.conf.in $@
