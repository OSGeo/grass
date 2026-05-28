file(GLOB dbmibase_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/dbmi_base/*.c")
if(MSVC)
  set(dbmibase_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/dbmi_base" "${CMAKE_SOURCE_DIR}/msvc")
  list(APPEND dbmibase_SRCS "${CMAKE_SOURCE_DIR}/msvc/dirent.c")
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
  grass_gis
  INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/dbmi_base")

build_library_in_subdir(stubs NAME grass_dbstubs DEPENDS grass_gis
                        grass_dbmibase)

build_library_in_subdir(
  dbmi_driver
  NAME
  grass_dbmidriver
  DEPENDS
  grass_dbmibase
  grass_dbstubs
  grass_gis
  INCLUDES
  "${CMAKE_CURRENT_SOURCE_DIR}/dbmi_base")

add_subdirectory(sqlp)
