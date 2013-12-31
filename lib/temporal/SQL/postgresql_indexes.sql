--#############################################################################
-- This SQL script generates the postgresql indexes
--
-- Author: Soeren Gebbert soerengebbert <at> googlemail <dot> com
--#############################################################################

CREATE INDEX raster_relative_time_index ON raster_relative_time (start_time, end_time);
CREATE INDEX raster_absolute_time_index ON raster_absolute_time (start_time, end_time);

CREATE INDEX raster3d_relative_time_index ON raster3d_relative_time (start_time, end_time);
CREATE INDEX raster3d_absolute_time_index ON raster3d_absolute_time (start_time, end_time);

CREATE INDEX vector_relative_time_index ON vector_relative_time (start_time, end_time);
CREATE INDEX vector_absolute_time_index ON vector_absolute_time (start_time, end_time);
