--#############################################################################
-- This SQL script generates the space time vector dataset metadata table,
-- view and trigger
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
-- Create the views to access all columns for absolute or relative time

CREATE VIEW stvds_view_abs_time AS SELECT 
            A1.id, A1.name, A1.mapset, A1.temporal_type,
            A1.semantic_type, 
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
	    A2.start_time, A2.end_time, A2.timezone,
            A2.granularity,
	    A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.vector_register,
	    A4.number_of_maps, 
            A4.title, A4.description, A4.command, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
	    FROM stvds_base A1, stvds_absolute_time A2,  
            stvds_spatial_extent A3, stvds_metadata A4 WHERE A1.id = A2.id AND 
	    A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW stvds_view_rel_time AS SELECT 
            A1.id, A1.name, A1.mapset, A1.temporal_type,
            A1.semantic_type, 
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
	    A2.start_time, A2.end_time, A2.granularity,
	    A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.vector_register,
	    A4.number_of_maps, 
            A4.title, A4.description, A4.command, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
	    FROM stvds_base A1, stvds_relative_time A2,  
            stvds_spatial_extent A3, stvds_metadata A4 WHERE A1.id = A2.id AND 
	    A1.id = A3.id AND A1.id = A4.id;


-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 
-- Uncommented due to performance issues
--CREATE TRIGGER update_stvds_metadata AFTER UPDATE ON stvds_metadata 
--  BEGIN
--    UPDATE stvds_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE stvds_base SET revision = (revision + 1) WHERE id = old.id;
--  END;
