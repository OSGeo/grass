set(dbf_SRCS
    dbf/column.c
    dbf/create_table.c
    dbf/cursor.c
    dbf/db.c
    dbf/dbfexe.c
    dbf/describe.c
    dbf/driver.c
    dbf/error.c
    dbf/execute.c
    dbf/fetch.c
    dbf/listtab.c
    dbf/main.c
    dbf/select.c
    dbf/str.c
    dbf/table.c)

set(mysql_SRCS
    mysql/create_table.c
    mysql/cursor.c
    mysql/dbe.c
    mysql/describe.c
    mysql/driver.c
    mysql/error.c
    mysql/execute.c
    mysql/fetch.c
    mysql/index.c
    mysql/listtab.c
    mysql/main.c
    mysql/parse.c
    mysql/replace.c
    mysql/select.c)

set(db_drivers)
set(grass_dbstubs_DEFS)
if(MSVC)
  set(grass_dbstubs_DEFS "-DDB_DRIVER_C=1")
endif()
build_program_in_subdir(
  dbf
  SOURCES
  ${dbf_SRCS}
  DEPENDS
  grass_gis
  grass_dbstubs
  grass_dbmibase
  grass_dbmidriver
  grass_shape
  grass_sqlp
  DEFS
  "${grass_dbstubs_DEFS}"
  HTML_FILE_NAME
  grass-dbf
  RUNTIME_OUTPUT_DIR
  "${GRASS_INSTALL_DRIVERDIR}/db")

list(APPEND db_drivers dbf)

build_program_in_subdir(
  ogr
  DEPENDS
  grass_gis
  grass_dbstubs
  grass_dbmibase
  grass_dbmidriver
  grass_sqlp
  PRIMARY_DEPENDS
  GDAL::GDAL
  DEFS
  "${grass_dbstubs_DEFS}"
  HTML_FILE_NAME
  grass-ogr
  RUNTIME_OUTPUT_DIR
  "${GRASS_INSTALL_DRIVERDIR}/db")

list(APPEND db_drivers ogr)

build_program_in_subdir(
  odbc
  DEPENDS
  grass_gis
  grass_dbstubs
  grass_dbmibase
  grass_dbmidriver
  grass_sqlp
  DEFS
  "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS
  ODBC::ODBC
  HTML_FILE_NAME
  grass-odbc
  RUNTIME_OUTPUT_DIR
  "${GRASS_INSTALL_DRIVERDIR}/db")
if(TARGET ODBC)
  list(APPEND db_drivers odbc)
endif()

build_program_in_subdir(
  sqlite
  DEPENDS
  grass_gis
  grass_dbstubs
  grass_dbmibase
  grass_dbmidriver
  grass_sqlp
  DEFS
  "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS
  SQLite::SQLite3
  HTML_FILE_NAME
  grass-sqlite
  RUNTIME_OUTPUT_DIR
  "${GRASS_INSTALL_DRIVERDIR}/db")

if(TARGET SQLite::SQLite3)
  list(APPEND db_drivers sqlite)
endif()

build_program_in_subdir(
  postgres
  NAME
  pg
  DEPENDS
  grass_gis
  grass_dbstubs
  grass_dbmibase
  grass_dbmidriver
  grass_sqlp
  DEFS
  "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS
  PostgreSQL::PostgreSQL
  HTML_FILE_NAME
  grass-pg
  RUNTIME_OUTPUT_DIR
  "${GRASS_INSTALL_DRIVERDIR}/db")

if(TARGET PostgreSQL::PostgreSQL)
  list(APPEND db_drivers pg)
endif()

build_program_in_subdir(
  mysql
  SOURCES
  ${mysql_SRCS}
  DEPENDS
  grass_gis
  grass_dbstubs
  grass_dbmibase
  grass_dbmidriver
  grass_sqlp
  DEFS
  "${grass_dbstubs_DEFS}"
  PRIMARY_DEPENDS
  MYSQL
  HTML_FILE_NAME
  grass-mysql
  RUNTIME_OUTPUT_DIR
  "${GRASS_INSTALL_DRIVERDIR}/db")

if(TARGET MYSQL)
  list(APPEND db_drivers mysql)

  if(WITH_DOCS)
    generate_html(TARGET mysql NAME grass-mesql SOURCEDIR
                  ${CMAKE_CURRENT_SOURCE_DIR}/mysql)
  endif()
endif()

set(db_drivers
    ${db_drivers}
    PARENT_SCOPE)
