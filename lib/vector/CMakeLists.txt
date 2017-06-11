# MODULE_TOPDIR = ../..

# PGM = vectorascii

# SUBDIRS = rtree \
# 	dglib \
# 	diglib \
# 	Vlib \
# 	vedit \
# 	neta

# include $(MODULE_TOPDIR)/include/Make/Dir.make
# include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# DOXNAME = vector

# default: htmldir

# htmldir: parsubdirs

# diglib: rtree

# Vlib: rtree dglib diglib

# vedit: Vlib

# neta: Vlib

#add_executable(vectorascii )
build_library_in_subdir(rtree HEADERS "rtree.h")

add_subdirectory(dglib)
add_subdirectory(diglib)

build_library_in_subdir(Vlib
  NAME grass_vector
  DEPENDS grass_gis grass_raster grass_rtree
  grass_graph grass_dig2 grass_dbmibase
  grass_btree2 grass_dbmiclient
  grass_linkm grass_gproj
  )

add_subdirectory(vedit)

add_subdirectory(neta)
