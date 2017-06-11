
set(db_drivers grass_gis)
add_subdirectory(drivers)

build_program_in_subdir(db.columns DEPENDS grass_dbmiclient)
add_dependencies(db.columns ${db_drivers})

build_program_in_subdir(db.createdb DEPENDS grass_dbmiclient)
add_dependencies(db.createdb ${db_drivers})

build_program_in_subdir(db.describe DEPENDS grass_dbmiclient)
add_dependencies(db.describe ${db_drivers})

build_program_in_subdir(db.connect DEPENDS grass_dbmiclient)
add_dependencies(db.connect ${db_drivers})

build_program_in_subdir(db.copy DEPENDS grass_dbmiclient)
add_dependencies(db.copy ${db_drivers})

build_program_in_subdir(db.execute DEPENDS grass_dbmiclient)
add_dependencies(db.execute ${db_drivers})

build_program_in_subdir(db.drivers DEPENDS grass_dbmiclient)
add_dependencies(db.drivers ${db_drivers})

build_program_in_subdir(db.databases DEPENDS grass_dbmiclient)
add_dependencies(db.databases ${db_drivers})

build_program_in_subdir(db.dropdb DEPENDS grass_dbmiclient)
add_dependencies(db.dropdb ${db_drivers})

build_program_in_subdir(db.login DEPENDS grass_dbmiclient)
add_dependencies(db.login ${db_drivers})

build_program_in_subdir(db.select DEPENDS grass_dbmiclient)
add_dependencies(db.select ${db_drivers})

build_program_in_subdir(db.tables DEPENDS grass_dbmiclient)
add_dependencies(db.tables ${db_drivers})

#TODO: PGM = databaseintro
