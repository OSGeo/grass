--#############################################################################
-- This SQL script generates the raster metadata table to store 
-- and metadata for SQL queries and temporal GIS support. Additionally two views
-- are created to access all map specific tables
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

PRAGMA foreign_keys = ON;

-- The metadata table reflects most of the raster metadata available in grass

CREATE TABLE  raster_metadata (
  id VARCHAR NOT NULL,                  -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  strds_register VARCHAR, -- The name of the table storing all space-time raster datasets in which this map is registered
  datatype VARCHAR NOT NULL,
  cols INTEGER NOT NULL,
  rows INTEGER NOT NULL,
  number_of_cells INTEGER NOT NULL,
  nsres DOUBLE PRECISION NOT NULL,
  ewres DOUBLE PRECISION NOT NULL,
  min DOUBLE PRECISION NOT NULL,
  max DOUBLE PRECISION NOT NULL,
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  raster_base (id) ON DELETE CASCADE
);

-- Create the views to access all cols for the absolute and relative time

CREATE VIEW raster_view_abs_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, A1.modification_time,
            A1.revision, A1.creator, 
	    A2.start_time, A2.end_time, 
            A3.north, A3.south, A3.east, A3.west,
	    A4.datatype, A4.cols, A4.rows,
            A4.nsres, A4.ewres, A4.min, A4.max,
	    A4.strds_register,
            A4.number_of_cells
	    FROM raster_base A1, raster_absolute_time A2, 
            raster_spatial_extent A3, raster_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW raster_view_rel_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, A1.modification_time,
            A1.revision, A1.creator, 
	    A2.interval,
            A3.north, A3.south, A3.east, A3.west,
	    A4.datatype, A4.cols, A4.rows,
            A4.nsres, A4.ewres, A4.min, A4.max,
	    A4.strds_register,
            A4.number_of_cells
	    FROM raster_base A1, raster_relative_time A2, 
            raster_spatial_extent A3, raster_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

-- Create a trigger to update the modification time and revision number in case the metadata have been updated 

CREATE TRIGGER update_raster_metadata AFTER UPDATE ON raster_metadata 
  BEGIN
    UPDATE raster_base SET modification_time = datetime("NOW") WHERE id = old.id;
    UPDATE raster_base SET revision = (revision + 1) WHERE id = old.id;
  END;

-- Create trigger for automated deletion of dependent rows, this should normally be done using foreign keys 

CREATE TRIGGER delete_raster_base AFTER DELETE ON raster_base
  BEGIN
    DELETE FROM raster_absolute_time WHERE id = old.id;
    DELETE FROM raster_relative_time WHERE id = old.id;
    DELETE FROM raster_spatial_extent WHERE id = old.id;
    DELETE FROM raster_metadata WHERE id = old.id;
  END;

