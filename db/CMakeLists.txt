
set(db_driver_targets)
add_subdirectory(drivers)

build_program_in_subdir(db.columns DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.columns ${db_driver_targets})

build_program_in_subdir(db.createdb DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.createdb ${db_driver_targets})

build_program_in_subdir(db.describe DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.describe ${db_driver_targets})

build_program_in_subdir(db.connect DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.connect ${db_driver_targets})

build_program_in_subdir(db.copy DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.copy ${db_driver_targets})

build_program_in_subdir(db.execute DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.execute ${db_driver_targets})

build_program_in_subdir(db.drivers DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.drivers ${db_driver_targets})

build_program_in_subdir(db.databases DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.databases ${db_driver_targets})

build_program_in_subdir(db.dropdb DEPENDS grass_gis grass_dbmiclient)
add_dependencies(db.dropdb ${db_driver_targets})

#TODO: PGM = databaseintro
