import pytest
from grass.jupyter import TimeSeriesMap

def test_timeseriesmap_render():
    """Test if TimeSeriesMap renders layers, overlays, and legends correctly."""
    
    # Create instance
    precip_map = TimeSeriesMap(use_region=True)
    
    # Add raster series
    precip_map.add_raster_series("precip_sum_2018")
    
    # Add overlays (boundary, scalebar, legend)
    precip_map.d_vect(map="boundary_country", fill_color="none")
    precip_map.d_barscale()
    precip_map.d_legend(color="black", at=(10, 40, 2, 6))
    
    # Render frames (headless)
    precip_map.render()
    
    # Verify layers
    assert precip_map._layers is not None, "No layers loaded"
    assert len(precip_map._layers) > 0, "Layers list is empty"
    
    # Verify overlays (d_vect and d_barscale)
    assert len(precip_map._base_calls) >= 2, (
        f"Expected ≥2 overlay calls, got {len(precip_map._base_calls)}"
    )
    
    # Verify legend configuration
    assert precip_map._legend == {
        "color": "black",
        "at": (10, 40, 2, 6),
    }, "Legend not configured correctly"
