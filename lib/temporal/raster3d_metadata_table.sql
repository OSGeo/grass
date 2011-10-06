--#############################################################################
-- This SQL script generates the raster3d metadata table to store 
-- and metadata for SQL queries and temporal GIS support. Additionally two views
-- are created to access all map specific tables
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

-- The metadata table reflects most of the raster3d metadata available in grass

CREATE TABLE  raster3d_metadata (
  id VARCHAR NOT NULL,                  -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  str3ds_register VARCHAR, -- The name of the table storing all space-time raster3d datasets in which this map is registered
  datatype VARCHAR NOT NULL,
  cols INTEGER NOT NULL,
  rows INTEGER NOT NULL,
  depths INTEGER NOT NULL,
  number_of_cells INTEGER NOT NULL,
  nsres DOUBLE PRECISION NOT NULL,
  ewres DOUBLE PRECISION NOT NULL,
  tbres DOUBLE PRECISION NOT NULL,
  min DOUBLE PRECISION NOT NULL,
  max DOUBLE PRECISION NOT NULL,
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  raster3d_base (id) ON DELETE CASCADE
);

-- Create the views to access all cols for the absolute and relative time

CREATE VIEW raster3d_view_abs_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator,
	    A2.start_time, A2.end_time, A2.timezone,
            A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.datatype, A4.cols, A4.rows, A4.depths,
            A4.nsres, A4.ewres, A4.tbres,
            A4.min, A4.max,
	    A4.str3ds_register,
            A4.number_of_cells
	    FROM raster3d_base A1, raster3d_absolute_time A2, 
            raster3d_spatial_extent A3, raster3d_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW raster3d_view_rel_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator,
	    A2.start_time, A2.end_time,
            A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.datatype, A4.cols, A4.rows, A4.depths,
            A4.nsres, A4.ewres, A4.tbres,
            A4.min, A4.max,
	    A4.str3ds_register,
            A4.number_of_cells
	    FROM raster3d_base A1, raster3d_relative_time A2, 
            raster3d_spatial_extent A3, raster3d_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

-- Create a trigger to update the modification time and revision number in case the metadata have been updated 

--CREATE TRIGGER update_raster3d_metadata AFTER UPDATE ON raster3d_metadata 
--  BEGIN
--    UPDATE raster3d_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE raster3d_base SET revision = (revision + 1) WHERE id = old.id;
--  END;
