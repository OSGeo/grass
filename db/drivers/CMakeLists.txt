set(dbf_SRCS dbf/column.c dbf/create_table.c
  dbf/cursor.c dbf/db.c dbf/dbfexe.c dbf/describe.c
  dbf/driver.c dbf/error.c dbf/execute.c dbf/fetch.c
  dbf/listtab.c dbf/main.c dbf/select.c dbf/str.c dbf/table.c)

set(db_drivers)
set(grass_dbstubs_DEFS)
if(MSVC)
set(grass_dbstubs_DEFS "-DDB_DRIVER_C=1")
endif()
build_program_in_subdir(dbf
  SOURCES ${dbf_SRCS}
  DEPENDS grass_gis grass_dbmidriver grass_shape grass_sqlp
  DEFS "${grass_dbstubs_DEFS}"
  HTML_FILE_NAME grass-dbf
  RUNTIME_OUTPUT_DIR "driver/db/")

list(APPEND db_drivers dbf)

build_program_in_subdir(ogr
  DEPENDS grass_gis grass_dbmidriver grass_sqlp GDAL
  DEFS "${grass_dbstubs_DEFS}"
  HTML_FILE_NAME grass-ogr
  RUNTIME_OUTPUT_DIR "driver/db/")

list(APPEND db_drivers ogr)

build_program_in_subdir(odbc
  DEPENDS grass_gis grass_dbmidriver grass_sqlp
  DEFS "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS ODBC
  HTML_FILE_NAME grass-odbc
  RUNTIME_OUTPUT_DIR "driver/db/")
if(TARGET ODBC)
  list(APPEND db_drivers odbc)
endif()

build_program_in_subdir(sqlite
  DEPENDS grass_gis grass_dbmidriver grass_sqlp
  DEFS "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS SQLITE
  HTML_FILE_NAME grass-sqlite
  RUNTIME_OUTPUT_DIR "driver/db/")

if(TARGET SQLITE)
  list(APPEND db_drivers sqlite)
endif()

build_program_in_subdir(postgres
  DEPENDS grass_gis grass_dbmidriver grass_sqlp
  DEFS "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS POSTGRES
  HTML_FILE_NAME grass-pg
  RUNTIME_OUTPUT_DIR "driver/db/")

if(TARGET POSTGRES)
  list(APPEND db_drivers postgres)
endif()


if(MYSQL_LIB)
  ##add_subdirectory(mysql)
  #list(APPEND db_drivers "mysql")
endif()


set(db_drivers ${db_drivers} PARENT_SCOPE)
