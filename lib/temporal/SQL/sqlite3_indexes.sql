--#############################################################################
-- This SQL script generates the sqlite3 indexes
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- Indexes for space time datasets

CREATE INDEX strds_base_index ON strds_base (id);
CREATE INDEX strds_relative_time_index ON strds_relative_time (id);
CREATE INDEX strds_absolute_time_index ON strds_absolute_time (id);
CREATE INDEX strds_spatial_extent_index ON strds_spatial_extent (id);

CREATE INDEX str3ds_base_index ON str3ds_base (id);
CREATE INDEX str3ds_relative_time_index ON str3ds_relative_time (id);
CREATE INDEX str3ds_absolute_time_index ON str3ds_absolute_time (id);
CREATE INDEX str3ds_spatial_extent_index ON str3ds_spatial_extent (id);

CREATE INDEX stvds_base_index ON stvds_base (id);
CREATE INDEX stvds_relative_time_index ON stvds_relative_time (id);
CREATE INDEX stvds_absolute_time_index ON stvds_absolute_time (id);
CREATE INDEX stvds_spatial_extent_index ON stvds_spatial_extent (id);

CREATE INDEX str3ds_metadata_index ON str3ds_metadata (id);
CREATE INDEX strds_metadata_index ON strds_metadata (id);
CREATE INDEX stvds_metadata_index ON stvds_metadata (id);

-- Indexes for raster, vector and 3D raster maps

CREATE INDEX raster_base_index ON raster_base (id);
CREATE INDEX raster_relative_time_index ON raster_relative_time (id, start_time, end_time);
CREATE INDEX raster_absolute_time_index ON raster_absolute_time (id, start_time, end_time);
CREATE INDEX raster_spatial_extent_index ON raster_spatial_extent (id);
CREATE INDEX raster_stds_register_index ON raster_stds_register (id);


CREATE INDEX raster3d_base_index ON raster3d_base (id);
CREATE INDEX raster3d_relative_time_index ON raster3d_relative_time (id, start_time, end_time);
CREATE INDEX raster3d_absolute_time_index ON raster3d_absolute_time (id, start_time, end_time);
CREATE INDEX raster3d_spatial_extent_index ON raster3d_spatial_extent (id);
CREATE INDEX raster3d_stds_register_index ON raster3d_stds_register (id);

CREATE INDEX vector_base_index ON vector_base (id);
CREATE INDEX vector_relative_time_index ON vector_relative_time (id, start_time, end_time);
CREATE INDEX vector_absolute_time_index ON vector_absolute_time (id, start_time, end_time);
CREATE INDEX vector_spatial_extent_index ON vector_spatial_extent (id);
CREATE INDEX vector_stds_register_index ON vector_stds_register (id);

CREATE INDEX raster3d_metadata_index ON raster3d_metadata (id);
CREATE INDEX raster_metadata_index ON raster_metadata (id);
CREATE INDEX vector_metadata_index ON vector_metadata (id);