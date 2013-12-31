--#############################################################################
-- Create trigger for automated deletion of dependent rows
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

CREATE OR REPLACE FUNCTION delete_strds_base() RETURNS TRIGGER AS $$
BEGIN
DELETE FROM strds_absolute_time WHERE id = OLD.id;
DELETE FROM strds_relative_time WHERE id = OLD.id;
DELETE FROM strds_spatial_extent WHERE id = OLD.id;
DELETE FROM strds_metadata WHERE id = OLD.id;
RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_raster_base() RETURNS TRIGGER AS $$
BEGIN
DELETE FROM raster_absolute_time WHERE id = OLD.id;
DELETE FROM raster_relative_time WHERE id = OLD.id;
DELETE FROM raster_spatial_extent WHERE id = OLD.id;
DELETE FROM raster_metadata WHERE id = OLD.id;
DELETE FROM raster_stds_register WHERE id = OLD.id;
RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_str3ds_base() RETURNS TRIGGER AS $$
BEGIN
DELETE FROM str3ds_absolute_time WHERE id = OLD.id;
DELETE FROM str3ds_relative_time WHERE id = OLD.id;
DELETE FROM str3ds_spatial_extent WHERE id = OLD.id;
DELETE FROM str3ds_metadata WHERE id = OLD.id;
RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_raster3d_base() RETURNS TRIGGER AS $$
BEGIN
DELETE FROM raster3d_absolute_time WHERE id = OLD.id;
DELETE FROM raster3d_relative_time WHERE id = OLD.id;
DELETE FROM raster3d_spatial_extent WHERE id = OLD.id;
DELETE FROM raster3d_metadata WHERE id = OLD.id;
DELETE FROM raster3d_stds_register WHERE id = OLD.id;
RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_stvds_base() RETURNS TRIGGER AS $$
BEGIN
DELETE FROM stvds_absolute_time WHERE id = OLD.id;
DELETE FROM stvds_relative_time WHERE id = OLD.id;
DELETE FROM stvds_spatial_extent WHERE id = OLD.id;
DELETE FROM stvds_metadata WHERE id = OLD.id;
RETURN OLD;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION delete_vector_base() RETURNS TRIGGER AS $$
BEGIN
DELETE FROM vector_absolute_time WHERE id = OLD.id;
DELETE FROM vector_relative_time WHERE id = OLD.id;
DELETE FROM vector_spatial_extent WHERE id = OLD.id;
DELETE FROM vector_metadata WHERE id = OLD.id;
DELETE FROM vector_stds_register WHERE id = OLD.id;
RETURN OLD;
END;
$$ LANGUAGE plpgsql;


CREATE TRIGGER delete_strds_base AFTER DELETE ON strds_base FOR EACH ROW EXECUTE PROCEDURE delete_strds_base();
CREATE TRIGGER delete_raster_base AFTER DELETE ON raster_base FOR EACH ROW EXECUTE PROCEDURE delete_raster_base();
CREATE TRIGGER delete_str3ds_base AFTER DELETE ON str3ds_base FOR EACH ROW EXECUTE PROCEDURE delete_str3ds_base();
CREATE TRIGGER delete_raster3d_base AFTER DELETE ON raster3d_base FOR EACH ROW EXECUTE PROCEDURE delete_raster3d_base();
CREATE TRIGGER delete_stvds_base AFTER DELETE ON stvds_base FOR EACH ROW EXECUTE PROCEDURE delete_stvds_base();
CREATE TRIGGER delete_vector_base AFTER DELETE ON vector_base FOR EACH ROW EXECUTE PROCEDURE delete_vector_base();

