set(dbf_SRCS dbf/column.c dbf/create_table.c
  dbf/cursor.c dbf/db.c dbf/dbfexe.c dbf/describe.c
  dbf/driver.c dbf/error.c dbf/execute.c dbf/fetch.c
  dbf/listtab.c dbf/main.c dbf/select.c dbf/str.c dbf/table.c)

build_program_in_subdir(dbf
  SOURCES ${dbf_SRCS}
  DEPENDS grass_gis grass_dbmidriver
  grass_shape grass_sqlp
  )


if(ODBC_LIB)
  add_subdirectory(drivers/odbc)
endif()

if(PQ_LIB)
  add_subdirectory(drivers/postgres)
endif()

if(MYSQL_LIB)
  add_subdirectory(drivers/mysql)
endif()

if(SQLITE_LIB)
  add_subdirectory(drivers/sqlite)
endif()

if(USE_OGR)
  add_subdirectory(drivers/ogr)
endif()
