--#############################################################################
-- This SQL script generates the table in which all registered GRASS_MAP maps 
-- are listed in.
--
-- This table will be created for each STDS.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_NAME is a placeholder for specific stds name (SQL compliant): name_mapset
-- SPACETIME_ID is a placeholder for specific stds id: name@mapset
-- GRASS_MAP is a placeholder for specific map type: raster, raster3d or vector
-- STDS is a placeholder for specific space-time dataset type: strds, str3ds, stvds

--PRAGMA foreign_keys = ON;

-- This table stores the.ids of the GRASS_MAP maps registered in the current spacetime GRASS_MAP table
CREATE TABLE  SPACETIME_NAME_GRASS_MAP_register (
  id VARCHAR NOT NULL, -- This colum is a primary foreign key storing the registered GRASS_MAP map.ids
  FOREIGN KEY (id) REFERENCES  GRASS_MAP_base (id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX SPACETIME_NAME_GRASS_MAP_register_index ON SPACETIME_NAME_GRASS_MAP_register (id);
