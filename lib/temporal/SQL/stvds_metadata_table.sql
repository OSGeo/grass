--#############################################################################
-- This SQL script generates the space time vector dataset metadata table.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

CREATE TABLE  stvds_metadata (
  id VARCHAR NOT NULL,    -- Name of the space-time vector dataset, this is the primary foreign key
  vector_register VARCHAR,-- The id of the table in which the vector maps are registered for this dataset
  number_of_maps INTEGER, -- The number of registered vector maps
  title VARCHAR,          -- Title of the space-time vector dataset
  description VARCHAR,    -- Detailed description of the space-time vector dataset
  command VARCHAR,        -- The command that was used to create the space time vector dataset
  points INTEGER,         -- The number of points accumulated from all registered maps
  lines INTEGER,          -- The number of lines accumulated from all registered maps
  boundaries INTEGER,     -- The number of boundaries accumulated from all registered maps
  centroids INTEGER,      -- The number of centroids accumulated from all registered maps
  faces INTEGER,          -- The number of faces accumulated from all registered maps
  kernels INTEGER,        -- The number of kernels accumulated from all registered maps
  primitives INTEGER,     -- All primitives accumulated (points, lines,boundaries,centroids,faces,kernels)
  nodes INTEGER,          -- Number of nodes accumulated from all registered maps (topological information) 
  areas INTEGER,          -- The number of areas accumulated from all registered maps (topological information)
  islands INTEGER,        -- The number of islands accumulated from all registered maps (topological information)
  holes INTEGER,          -- The number of holes accumulated from all registered maps (topological information)
  volumes INTEGER,        -- The number of volumes accumulated from all registered maps (topological information)
  PRIMARY KEY (id),  
  FOREIGN KEY (id) REFERENCES  stvds_base (id) ON DELETE CASCADE
);

-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 
-- Uncommented due to performance issues
--CREATE TRIGGER update_stvds_metadata AFTER UPDATE ON stvds_metadata 
--  BEGIN
--    UPDATE stvds_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE stvds_base SET revision = (revision + 1) WHERE id = old.id;
--  END;
