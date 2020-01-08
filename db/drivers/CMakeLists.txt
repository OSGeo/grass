set(dbf_SRCS dbf/column.c dbf/create_table.c
  dbf/cursor.c dbf/db.c dbf/dbfexe.c dbf/describe.c
  dbf/driver.c dbf/error.c dbf/execute.c dbf/fetch.c
  dbf/listtab.c dbf/main.c dbf/select.c dbf/str.c dbf/table.c)

build_program_in_subdir(dbf
  SOURCES ${dbf_SRCS}
  DEPENDS grass_gis grass_dbmidriver
  grass_shape grass_sqlp
  HTML_FILE_NAME grass-dbf
  RUNTIME_OUTPUT_DIR "driver/db/"
  )

set(db_drivers)
list(APPEND db_drivers dbf)
#TODO ; add targets
if(ODBC_LIB)
  add_subdirectory(odbc)
  #list(APPEND db_drivers "odbc")
endif()

if(MYSQL_LIB)
  add_subdirectory(mysql)
  #list(APPEND db_drivers "mysql")
endif()

if(TARGET POSTGRES)
  add_subdirectory(postgres)
  #list(APPEND db_drivers "postrgresq")
endif()

if(TARGET SQLITE)
  list(APPEND db_drivers sqlite)
  build_program_in_subdir(sqlite
  DEPENDS grass_gis grass_dbmidriver
  grass_sqlp SQLITE
  HTML_FILE_NAME grass-sqlite
  RUNTIME_OUTPUT_DIR "driver/db/"
  )
endif()

if(TARGET GDAL)
  list(APPEND db_drivers ogr)
  build_program_in_subdir(ogr
  DEPENDS grass_gis grass_dbmidriver GDAL
  grass_sqlp
  HTML_FILE_NAME grass-ogr
  RUNTIME_OUTPUT_DIR "driver/db/")
endif()

set(db_drivers ${db_drivers} PARENT_SCOPE)
