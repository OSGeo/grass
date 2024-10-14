set(database_modules_list
    db.columns
    db.connect
    db.copy
    db.createdb
    db.databases
    db.describe
    db.drivers
    db.dropdb
    db.execute
    db.login
    db.select
    db.tables)

add_custom_target(
  ALL_DATABASE_MODULES ALL
  DEPENDS ${db_modules_list}
  COMMENT "All database modules are built.")

if(WITH_DOCS)
  generate_html(TARGET ALL_DATABASE_MODULES NAME databaseintro)
endif()

set(db_drivers grass_gis)
add_subdirectory(drivers)

build_program_in_subdir(db.columns DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.columns ${db_drivers})

build_program_in_subdir(db.createdb DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.createdb ${db_drivers})

build_program_in_subdir(db.describe DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver grass_parson)
add_dependencies(db.describe ${db_drivers})

build_program_in_subdir(db.connect DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.connect ${db_drivers})

build_program_in_subdir(db.copy DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.copy ${db_drivers})

build_program_in_subdir(db.execute DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.execute ${db_drivers})

build_program_in_subdir(db.drivers DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.drivers ${db_drivers})

build_program_in_subdir(db.databases DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.databases ${db_drivers})

build_program_in_subdir(db.dropdb DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.dropdb ${db_drivers})

build_program_in_subdir(db.login DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.login ${db_drivers})

build_program_in_subdir(db.select DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.select ${db_drivers})

build_program_in_subdir(db.tables DEPENDS grass_gis grass_dbmibase
                        grass_dbmiclient grass_dbmidriver)
add_dependencies(db.tables ${db_drivers})
