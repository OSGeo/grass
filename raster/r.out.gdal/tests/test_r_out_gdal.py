import grass.script as gs
import pytest
from grass.tools import Tools

FORMAT_DICT = {
    "COG": "tif",
    "GTiff": "tif",
    "GPKG": "gpkg",
    "VRT": "vrt",
    "netCDF": "nc",
    "ENVI": "img",
}


@pytest.fixture
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

    with gs.setup.init(project) as session:
        tools = Tools(session=session)
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
    tools = Tools(session=session)
    suffix = FORMAT_DICT[file_format]
    output_file = tmp_path / f"{mapname}.{suffix}"
    tools.r_out_gdal(
        input=mapname,
        output=str(output_file),
        format=file_format,
    )
    assert output_file.exists(), f"{mapname} not exportd to {output_file}"


def test_compressed_gtiff_export(simple_raster_map):
    """Test export of compressed GeoTiff using 'r.out.gdal'.

    Verifies:
    - Export of common format works.
    """
    mapname, session, tmp_path = simple_raster_map
    tools = Tools(session=session)

    output_file = tmp_path / f"{mapname}.tif"
    tools.r_out_gdal(
        input=mapname,
        output=str(output_file),
        format="GTiff",
        createopt="COMPRESS=LZW",
    )
    assert output_file.exists(), f"{mapname} not exportd to {output_file}"


def test_fast_gtiff_export(simple_raster_map):
    """Test fast export of compressed GeoTiff using 'r.out.gdal'.

    Verifies:
    - Fast export of works if type, nodata and -f are given.
    """
    mapname, session, tmp_path = simple_raster_map
    tools = Tools(session=session)

    output_file = tmp_path / f"{mapname}.tif"
    tools.r_out_gdal(
        flags="f",
        input=mapname,
        output=str(output_file),
        format="GTiff",
        type="Byte",
        nodata=255,
        createopt="COMPRESS=LZW",
    )
    assert output_file.exists(), f"{mapname} not exportd to {output_file}"
    # Here I would like to assert "Checking GDAL data type and nodata value" not in std_err
