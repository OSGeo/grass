--#############################################################################
-- This SQL script generates the space time vector dataset metadata table,
-- view and trigger
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

PRAGMA foreign_keys = ON;

CREATE TABLE  stvds_metadata (
  id VARCHAR NOT NULL,                -- Name of the space-time vector dataset, this is the primary foreign key
  vector_register VARCHAR,                    -- The id of the table in which the vector maps are registered for this dataset
  number_of_maps INTEGER,          -- The number of registered vector maps
  title VARCHAR,                              -- Title of the space-time vector dataset
  description VARCHAR,                        -- Detailed description of the space-time vector dataset
  PRIMARY KEY (id),  
  FOREIGN KEY (id) REFERENCES  stvds_base (id) ON DELETE CASCADE
);
-- Create the views to access all columns for absolute or relative time

CREATE VIEW stvds_view_abs_time AS SELECT 
            A1.id, A1.temporal_type,
            A1.creator, A1.semantic_type,  
            A1.creation_time, A1.modification_time,
            A1.revision, A2.start_time,
	    A2.end_time, A2.granularity,
	    A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.vector_register,
	    A4.number_of_maps, 
            A4.title, A4.description	
	    FROM stvds_base A1, stvds_absolute_time A2,  
            stvds_spatial_extent A3, stvds_metadata A4 WHERE A1.id = A2.id AND 
	    A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW stvds_view_rel_time AS SELECT 
            A1.id, A1.temporal_type,
            A1.creator, A1.semantic_type,  
            A1.creation_time, A1.modification_time,
            A1.revision, 
	    A2.interval, A2.granularity,
	    A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.vector_register,
	    A4.number_of_maps, 
            A4.title, A4.description	
	    FROM stvds_base A1, stvds_relative_time A2,  
            stvds_spatial_extent A3, stvds_metadata A4 WHERE A1.id = A2.id AND 
	    A1.id = A3.id AND A1.id = A4.id;


-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 

CREATE TRIGGER update_stvds_metadata AFTER UPDATE ON stvds_metadata 
  BEGIN
    UPDATE stvds_base SET modification_time = datetime("NOW") WHERE id = old.id;
    UPDATE stvds_base SET revision = (revision + 1) WHERE id = old.id;
  END;

-- Create trigger for automated deletion of dependent rows, this should normally be done using foreign keys 

CREATE TRIGGER delete_stvds_base AFTER DELETE ON stvds_base
  BEGIN
    DELETE FROM stvds_absolute_time WHERE id = old.id;
    DELETE FROM stvds_relative_time WHERE id = old.id;
    DELETE FROM stvds_spatial_exntent WHERE id = old.id;
    DELETE FROM stvds_metadata WHERE id = old.id;
  END;

