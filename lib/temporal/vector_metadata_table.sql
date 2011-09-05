--#############################################################################
-- This SQL script generates the vector table to store 
-- metadata for SQL queries and temporal GIS support.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

PRAGMA foreign_keys = ON;

-- The metadata table 

CREATE TABLE  vector_metadata (
  id VARCHAR NOT NULL,                  -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  stvds_register VARCHAR, -- The name of the table storing all space-time vector datasets in which this map is registered
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  vector_base (id) ON DELETE CASCADE
);

-- Create the views to access all columns for the absolute and relative time

CREATE VIEW vector_view_abs_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, A1.modification_time,
            A1.revision, A1.creator, 
	    A2.start_time, A2.end_time, 
            A3.north, A3.south, A3.east, A3.west,
	    A4.stvds_register
	    FROM vector_base A1, vector_absolute_time A2, 
            vector_spatial_extent A3, vector_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW vector_view_rel_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.temporal_type,
            A1.creation_time, A1.modification_time,
            A1.revision, A1.creator, 
	    A2.interval,
            A3.north, A3.south, A3.east, A3.west,
	    A4.stvds_register
	    FROM vector_base A1, vector_relative_time A2, 
            vector_spatial_extent A3, vector_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 

CREATE TRIGGER update_vector_metadata AFTER UPDATE ON vector_metadata 
  BEGIN
    UPDATE vector_base SET modification_time = datetime("NOW") WHERE id = old.id;
    UPDATE vector_base SET revision = (revision + 1) WHERE id = old.id;
  END;

-- Create trigger for automated deletion of dependent rows, this should normally be done using foreign keys 

CREATE TRIGGER delete_vector_base AFTER DELETE ON vector_base
  BEGIN
    DELETE FROM vector_absolute_time WHERE id = old.id;
    DELETE FROM vector_relative_time WHERE id = old.id;
    DELETE FROM vector_spatial_extent WHERE id = old.id;
    DELETE FROM vector_metadata WHERE id = old.id;
  END;

