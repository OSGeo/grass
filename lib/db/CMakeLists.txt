
# MODULE_TOPDIR = ../..

# #the order is relevant
# SUBDIRS = \
# 	dbmi_base \
# 	dbmi_client \
# 	stubs \
# 	dbmi_driver \
# 	sqlp

# #doxygen:
# DOXNAME=dbmi

# include $(MODULE_TOPDIR)/include/Make/Dir.make
# include $(MODULE_TOPDIR)/include/Make/Doxygen.make

# default: parsubdirs


# stubs: dbmi_base

# dbmi_client: dbmi_base


build_library_in_subdir(dbmi_base NAME grass_dbmibase
  HEADERS "dbstubs.h")

build_library_in_subdir(dbmi_client NAME grass_dbmiclient
  DEPENDS grass_dbmibase
  INCLUDES "./dbmi_base"
  )

build_library_in_subdir(stubs NAME grass_dstubs
  DEPENDS grass_dbmibase
  )

build_library_in_subdir(dbmi_driver NAME grass_dbmidriver
  DEPENDS grass_dstubs
  INCLUDES "./dbmi_base"
  )


add_subdirectory(sqlp)
