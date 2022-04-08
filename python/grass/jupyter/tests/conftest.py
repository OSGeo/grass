"""Fixture for ReprojectionRenderer test with simple GRASS location, raster, vector"""

from types import SimpleNamespace

import pytest

import grass.script as gs
import grass.script.setup as grass_setup


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Start a session and create a raster time series
    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.proj", flags="c", epsg=26917)
        gs.run_command("g.region", s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        # Vector
        vector_name = "point"
        full_vector_name = f"{vector_name}@PERMANENT"
        gs.write_command(
            "v.in.ascii", input="-", stdin="50|50", output=vector_name
        )
        # Random Raster
        raster_name = "precipitation"
        max_value = 100
        gs.mapcalc(f"{raster_name} = rand(0, {max_value})", seed=1)
        full_raster_name = f"{raster_name}@PERMANENT"
        yield SimpleNamespace(
            raster_name=raster_name,
            full_raster_name=full_raster_name,
            vector_name=vector_name,
            full_vector_name=full_vector_name,
        )
