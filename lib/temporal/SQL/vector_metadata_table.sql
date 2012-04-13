--#############################################################################
-- This SQL script generates the vector table to store 
-- metadata for SQL queries and temporal GIS support.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

-- The metadata table 

CREATE TABLE  vector_metadata (
  id VARCHAR NOT NULL,    -- The id (PFK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary foreign key
  stvds_register VARCHAR, -- The name of the table storing all space-time vector datasets in which this map is registered
  is_3d INTEGER,          -- This is 1 if the vector map is 3d and 0 otherwise 
  points INTEGER,         -- The number of points
  lines INTEGER,          -- The number of lines
  boundaries INTEGER,     -- The number of boundaries
  centroids INTEGER,      -- The number of centroids
  faces INTEGER,          -- The number of faces
  kernels INTEGER,        -- The number of kernels
  primitives INTEGER,     -- All primitives accumulated (points, lines,boundaries,centroids,faces,kernels)
  nodes INTEGER,          -- Number of nodes (topological information)
  areas INTEGER,          -- The number of areas (topological information)
  islands INTEGER,        -- The number of islands (topological information)
  holes INTEGER,          -- The number of holes (topological information)
  volumes INTEGER,        -- The number of volumes (topological information)
  PRIMARY KEY (id),
  FOREIGN KEY (id) REFERENCES  vector_base (id) ON DELETE CASCADE
);

-- Create the views to access all columns for the absolute and relative time

CREATE VIEW vector_view_abs_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.layer, A1.temporal_type,
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
	    A2.start_time, A2.end_time, A2.timezone,
            A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.stvds_register, A4.is_3d, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
	    FROM vector_base A1, vector_absolute_time A2, 
            vector_spatial_extent A3, vector_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

CREATE VIEW vector_view_rel_time AS SELECT 
            A1.id, A1.mapset,
            A1.name, A1.layer, A1.temporal_type,
            A1.creation_time, 
-- Uncommented due to performance issues
--            A1.modification_time, A1.revision, 
            A1.creator, 
	    A2.start_time, A2.end_time,
            A3.north, A3.south, A3.east, A3.west, A3.proj,
	    A4.stvds_register, A4.is_3d, A4.points, A4.lines,
            A4.boundaries, A4.centroids, A4.faces, A4.kernels,
            A4.primitives, A4.nodes, A4.areas, A4.islands,
            A4.holes, A4.volumes
	    FROM vector_base A1, vector_relative_time A2, 
            vector_spatial_extent A3, vector_metadata A4 
	    WHERE A1.id = A2.id AND A1.id = A3.id AND A1.id = A4.id;

-- Create a trigger to update the modification time and revision number in case the metadata or timestanps have been updated 
-- Uncommented due to performance issues
--CREATE TRIGGER update_vector_metadata AFTER UPDATE ON vector_metadata 
--  BEGIN
--    UPDATE vector_base SET modification_time = datetime("NOW") WHERE id = old.id;
--    UPDATE vector_base SET revision = (revision + 1) WHERE id = old.id;
--  END;
