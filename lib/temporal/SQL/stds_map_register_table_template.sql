--#############################################################################
-- This SQL script generates the table in which all registered GRASS_MAP maps 
-- are listed in.
--
-- This table will be created for each STDS.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- SPACETIME_NAME is a placeholder for specific stds name (SQL compliant): name_mapset
-- GRASS_MAP is a placeholder for specific map type: raster, raster3d or vector

-- This table stores the.ids of the GRASS_MAP maps registered in the current spacetime GRASS_MAP table
CREATE TABLE  SPACETIME_REGISTER_TABLE (
  id VARCHAR NOT NULL, -- This column is a primary key storing the registered GRASS_MAP map.ids
  PRIMARY KEY (id)
);
