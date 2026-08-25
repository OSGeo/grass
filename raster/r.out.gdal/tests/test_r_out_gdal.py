import os

import pytest

import grass.script as gs
from grass.tools import Tools

FORMAT_DICT = {
    "COG": "tif",
    "GPKG": "gpkg",
    "GTiff": "tif",
    "VRT": "vrt",
    "netCDF": "nc",
    "ENVI": "img",
}


@pytest.fixture(scope="module")
def simple_raster_map(tmp_path_factory):
    """Fixture to create a basic GRASS environment with a simple raster map containing attributes.

    Set up a temporary GRASS project and region and create a raster map
    using r.mapcalc.

    Yields:
        tuple: (map name, GRASS session handle, tmp_path)

    """
    tmp_path = tmp_path_factory.mktemp("r_out_gdal_project")
    project = tmp_path / "grassdata"
    gs.create_project(project, epsg=4326)

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session, consistent_return_value=True)
        tools.g_region(n=10, s=0, e=10, w=0, res=1)

        tools.r_mapcalc(
            expression="test_raster=row() * col()",
        )

        yield "test_raster", session, tmp_path


@pytest.mark.parametrize("file_format", list(FORMAT_DICT.keys()))
def test_basic_cog_export(simple_raster_map, file_format):
    """Test basic export of various formats using 'r.out.gdal'.

    Verifies:
    - Export of common format works with defaults.
    """
    mapname, session, tmp_path = simple_raster_map
    tools = Tools(session=session, consistent_return_value=True)
    suffix = FORMAT_DICT[file_format]
    output_file = tmp_path / f"{mapname}_{file_format}.{suffix}"
    export = tools.r_out_gdal(
        input=mapname,
        output=output_file,
        format=file_format,
    )
    # Check successful export
    assert output_file.exists(), f"{mapname} not exported to {output_file}"
    # Check that nodata and data range checks are performed
    assert "Checking GDAL data type and nodata value" in export.stderr


def test_compressed_gtiff_export(simple_raster_map):
    """Test export of compressed GeoTiff using 'r.out.gdal'.

    Verifies:
    - Export of common format works.
    """
    mapname, session, tmp_path = simple_raster_map
    tools = Tools(session=session, consistent_return_value=True)

    output_file = tmp_path / f"{mapname}_lzw.tif"
    export = tools.r_out_gdal(
        input=mapname,
        output=output_file,
        format="GTiff",
        createopt="COMPRESS=LZW",
    )
    # Check successful export
    assert output_file.exists(), f"{mapname} not exported to {output_file}"
    # Check that nodata and data range checks are performed
    assert "Checking GDAL data type and nodata value" in export.stderr


def test_fast_gtiff_export(simple_raster_map):
    """Test fast export of compressed GeoTiff using 'r.out.gdal'.

    Verifies:
    - Fast export of works if type, nodata and -f are given.
    """
    mapname, session, tmp_path = simple_raster_map
    tools = Tools(session=session, consistent_return_value=True)

    output_file = tmp_path / f"{mapname}_fast.tif"
    export = tools.r_out_gdal(
        flags="f",
        input=mapname,
        output=output_file,
        format="GTiff",
        type="Byte",
        nodata=255,
        createopt="COMPRESS=LZW",
    )
    # Check successful export
    assert output_file.exists(), f"{mapname} not exported to {output_file}"
    # Check that nodata and data range checks are NOT performed
    assert "Checking GDAL data type and nodata value" not in export.stderr
