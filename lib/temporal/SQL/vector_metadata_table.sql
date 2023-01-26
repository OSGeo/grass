--#############################################################################
-- This SQL script generates the vector table to store
-- metadata for SQL queries and temporal GIS support.
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

--PRAGMA foreign_keys = ON;

-- The metadata table

CREATE TABLE  vector_metadata (
  id VARCHAR NOT NULL,    -- The id (PK) is the unique identifier for all tables, it is based on name and mapset (name@mapset) and is used as primary key
  is_3d BOOLEAN,          -- This is 1 if the vector map is 3d and 0 otherwise
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
  PRIMARY KEY (id)
);
