file(GLOB dbmibase_SRCS "./dbmi_base/*.c")
if(MSVC)
  set(dbmibase_INCLUDES "./dbmi_base" "./dbmi_base/msvc")
  list(APPEND dbmibase_SRCS "./dbmi_base/msvc/dirent.c")
endif()

build_library_in_subdir(
  dbmi_base
  NAME
  grass_dbmibase
  INCLUDES
  ${dbmibase_INCLUDES}
  SOURCES
  ${dbmibase_SRCS}
  DEPENDS
  grass_gis
  HEADERS
  "dbstubs.h")

build_library_in_subdir(
  dbmi_client
  NAME
  grass_dbmiclient
  DEPENDS
  grass_dbmibase
  INCLUDES
  "./dbmi_base")

build_library_in_subdir(stubs NAME grass_dstubs DEPENDS grass_dbmibase)

build_library_in_subdir(
  dbmi_driver
  NAME
  grass_dbmidriver
  DEPENDS
  grass_dstubs
  INCLUDES
  "./dbmi_base")

add_subdirectory(sqlp)
