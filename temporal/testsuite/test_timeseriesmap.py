import pytest
from grass.jupyter import TimeSeriesMap

def test_timeseriesmap_render():
    """Test if TimeSeriesMap renders layers correctly."""

    # Create TimeSeriesMap instance
    precip_map = TimeSeriesMap(use_region=True)

    # Add raster series
    precip_map.add_raster_series("precip_sum_2018")

    # Add overlays (Legend, Boundary, and Scale bar)
    precip_map.d_legend(color="black", at=(10, 40, 2, 6))
    precip_map.d_vect(map="boundary_country", fill_color="none")
    precip_map.d_barscale()

    # Render the map
    precip_map.show()

    # Assert that the map object has layers
    assert precip_map._layers is not None
    assert len(precip_map._layers) > 0
