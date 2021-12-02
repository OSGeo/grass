--#############################################################################
-- This SQL script upgrades TGIS DB from version 2 to version 3.
--
-- Author: Martin Landa landa.martin <at> gmail <dot> com
--#############################################################################

-- raster_metadata_table.sql
ALTER TABLE raster_metadata
 ADD COLUMN semantic_label VARCHAR;

-- strds_metadata_table.sql
ALTER TABLE strds_metadata
 ADD COLUMN number_of_bands INTEGER;

-- tgis_metadata
UPDATE tgis_metadata
  SET value = '3'
  WHERE key = 'tgis_db_version';
