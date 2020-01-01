build_program_in_subdir(db.columns DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.createdb DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.describe DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.connect DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.copy DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.execute DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.drivers DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.databases DEPENDS grass_gis grass_dbmiclient )

build_program_in_subdir(db.dropdb DEPENDS grass_gis grass_dbmiclient )

# TODO: MODULES DISABLED
#add_subdirectory(dbf)
#add_subdirectory(drivers)


# MODULE_TOPDIR = ..

# SUBDIRS = \
# 	db.columns \
# 	db.connect \
# 	db.copy \
# 	db.createdb \
# 	db.databases \
# 	db.describe \
# 	db.drivers \
# 	db.dropdb \
# 	db.execute \
# 	db.login \
# 	db.select \
# 	db.tables

# CLEAN_SUBDIRS = drivers

# PGM = databaseintro

# include $(MODULE_TOPDIR)/include/Make/Dir.make

# default: htmldir

# htmldir:
# 	$(MAKE) -C drivers
# 	$(MAKE) parsubdirs

