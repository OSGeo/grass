--#############################################################################
-- This SQL script generates the sqlite3 trigger
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

-- Create trigger for automated deletion of dependent rows

CREATE TRIGGER delete_strds_base AFTER DELETE ON strds_base
  BEGIN
    DELETE FROM strds_absolute_time WHERE id = old.id;
    DELETE FROM strds_relative_time WHERE id = old.id;
    DELETE FROM strds_spatial_extent WHERE id = old.id;
    DELETE FROM strds_metadata WHERE id = old.id;
  END;

CREATE TRIGGER delete_raster_base AFTER DELETE ON raster_base
  BEGIN
    DELETE FROM raster_absolute_time WHERE id = old.id;
    DELETE FROM raster_relative_time WHERE id = old.id;
    DELETE FROM raster_spatial_extent WHERE id = old.id;
    DELETE FROM raster_metadata WHERE id = old.id;
    DELETE FROM raster_stds_register WHERE id = old.id;
  END;

CREATE TRIGGER delete_str3ds_base AFTER DELETE ON str3ds_base
  BEGIN
    DELETE FROM str3ds_absolute_time WHERE id = old.id;
    DELETE FROM str3ds_relative_time WHERE id = old.id;
    DELETE FROM str3ds_spatial_extent WHERE id = old.id;
    DELETE FROM str3ds_metadata WHERE id = old.id;
  END;

CREATE TRIGGER delete_raster3d_base AFTER DELETE ON raster3d_base
  BEGIN
    DELETE FROM raster3d_absolute_time WHERE id = old.id;
    DELETE FROM raster3d_relative_time WHERE id = old.id;
    DELETE FROM raster3d_spatial_extent WHERE id = old.id;
    DELETE FROM raster3d_metadata WHERE id = old.id;
    DELETE FROM raster3d_stds_register WHERE id = old.id;
  END;

CREATE TRIGGER delete_stvds_base AFTER DELETE ON stvds_base
  BEGIN
    DELETE FROM stvds_absolute_time WHERE id = old.id;
    DELETE FROM stvds_relative_time WHERE id = old.id;
    DELETE FROM stvds_spatial_extent WHERE id = old.id;
    DELETE FROM stvds_metadata WHERE id = old.id;
  END;

CREATE TRIGGER delete_vector_base AFTER DELETE ON vector_base
  BEGIN
    DELETE FROM vector_absolute_time WHERE id = old.id;
    DELETE FROM vector_relative_time WHERE id = old.id;
    DELETE FROM vector_spatial_extent WHERE id = old.id;
    DELETE FROM vector_metadata WHERE id = old.id;
    DELETE FROM vector_stds_register WHERE id = old.id;
  END;


