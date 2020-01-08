
set(db_drivers)
add_subdirectory(drivers)

build_program_in_subdir(db.columns DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.columns ${db_drivers})

build_program_in_subdir(db.createdb DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.createdb ${db_drivers})

build_program_in_subdir(db.describe DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.describe ${db_drivers})

build_program_in_subdir(db.connect DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.connect ${db_drivers})

build_program_in_subdir(db.copy DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.copy ${db_drivers})

build_program_in_subdir(db.execute DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.execute ${db_drivers})

build_program_in_subdir(db.drivers DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.drivers ${db_drivers})

build_program_in_subdir(db.databases DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.databases ${db_drivers})

build_program_in_subdir(db.dropdb DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.dropdb ${db_drivers})

#TODO: PGM = databaseintro
