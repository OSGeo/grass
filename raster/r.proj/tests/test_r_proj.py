"""Test r.proj module with basic CRS transformations.

This test suite validates r.proj functionality including:
- Basic raster reprojection
- Different interpolation methods
- Invalid input handling
- Region preservation

Author: [Your Name]
Date: 2026
"""

import pytest
import grass.script as gs


@pytest.fixture
def simple_raster(tmp_path):
    """Create a simple test raster with known values."""
    # Set a basic region
    gs.run_command("g.region", n=50, s=0, e=50, w=0, res=1)
    
    # Create a gradient raster for testing
    raster_name = "test_input_raster"
    gs.run_command(
        "r.mapcalc",
        expression=f"{raster_name} = row() + col()",
        overwrite=True
    )
    
    yield raster_name
    
    # Cleanup
    gs.run_command("g.remove", type="raster", name=raster_name, flags="f", quiet=True)


class TestRProjBasic:
    """Basic functionality tests for r.proj."""
    
    def test_same_projection_identity_transform(self, simple_raster):
        """Test r.proj with identical source and target projections."""
        output_name = "test_output_same_proj"
        
        try:
            # Run r.proj with same projection (identity transform)
            gs.run_command(
                "r.proj",
                input=simple_raster,
                output=output_name,
                location=gs.gisenv()["LOCATION_NAME"],
                mapset=gs.gisenv()["MAPSET"],
                overwrite=True,
                quiet=True
            )
            
            # Verify output exists
            assert gs.find_file(output_name, element="raster")["name"], \
                "Output raster should exist"
            
            # Check statistics are similar
            stats_input = gs.parse_command("r.univar", map=simple_raster, flags="g")
            stats_output = gs.parse_command("r.univar", map=output_name, flags="g")
            
            assert abs(float(stats_input["mean"]) - float(stats_output["mean"])) < 0.1, \
                "Mean values should be very similar for identity transform"
                
        finally:
            gs.run_command("g.remove", type="raster", name=output_name, flags="f", quiet=True)
    
    def test_invalid_location_fails(self, simple_raster):
        """Test that r.proj fails gracefully with non-existent location."""
        output_name = "test_output_invalid"
        
        with pytest.raises(Exception):
            gs.run_command(
                "r.proj",
                input=simple_raster,
                output=output_name,
                location="nonexistent_location_xyz123",
                mapset="PERMANENT",
                quiet=True
            )
    
    def test_output_respects_computational_region(self, simple_raster):
        """Test that r.proj respects the current computational region."""
        output_name = "test_output_region"
        
        # Set a specific region smaller than input
        gs.run_command("g.region", n=30, s=10, e=40, w=5, res=2)
        
        try:
            gs.run_command(
                "r.proj",
                input=simple_raster,
                output=output_name,
                location=gs.gisenv()["LOCATION_NAME"],
                mapset=gs.gisenv()["MAPSET"],
                overwrite=True,
                quiet=True
            )
            
            # Get output info
            output_info = gs.raster_info(output_name)
            current_region = gs.region()
            
            # Verify output matches region
            assert output_info["north"] == pytest.approx(current_region["n"], abs=0.01)
            assert output_info["south"] == pytest.approx(current_region["s"], abs=0.01)
            assert output_info["east"] == pytest.approx(current_region["e"], abs=0.01)
            assert output_info["west"] == pytest.approx(current_region["w"], abs=0.01)
            
        finally:
            gs.run_command("g.remove", type="raster", name=output_name, flags="f", quiet=True)


class TestRProjInterpolation:
    """Test different interpolation methods in r.proj."""
    
    @pytest.fixture
    def binary_raster(self):
        """Create a binary test raster with distinct values."""
        gs.run_command("g.region", n=20, s=0, e=20, w=0, res=1)
        
        raster_name = "test_binary_raster"
        # Create a raster with only 100 and 200 values
        gs.run_command(
            "r.mapcalc",
            expression=f"{raster_name} = if(row() < 10, 100, 200)",
            overwrite=True
        )
        
        yield raster_name
        
        gs.run_command("g.remove", type="raster", name=raster_name, flags="f", quiet=True)
    
    @pytest.mark.parametrize("method", ["nearest", "bilinear", "bicubic"])
    def test_interpolation_method_runs(self, binary_raster, method):
        """Test that all interpolation methods execute successfully."""
        output_name = f"test_output_{method}"
        
        try:
            gs.run_command(
                "r.proj",
                input=binary_raster,
                output=output_name,
                location=gs.gisenv()["LOCATION_NAME"],
                mapset=gs.gisenv()["MAPSET"],
                method=method,
                overwrite=True,
                quiet=True
            )
            
            # Verify output exists
            assert gs.find_file(output_name, element="raster")["name"], \
                f"Output with {method} interpolation should exist"
                
        finally:
            gs.run_command("g.remove", type="raster", name=output_name, flags="f", quiet=True)
    
    def test_nearest_neighbor_preserves_values(self, binary_raster):
        """Test that nearest neighbor doesn't interpolate values."""
        output_name = "test_nearest_values"
        
        try:
            gs.run_command(
                "r.proj",
                input=binary_raster,
                output=output_name,
                location=gs.gisenv()["LOCATION_NAME"],
                mapset=gs.gisenv()["MAPSET"],
                method="nearest",
                overwrite=True,
                quiet=True
            )
            
            # Get statistics
            stats = gs.parse_command("r.univar", map=output_name, flags="g")
            min_val = float(stats["min"])
            max_val = float(stats["max"])
            
            # Nearest neighbor should only have original values (100, 200)
            # with possible NULL values, but no interpolated values between
            assert min_val >= 100.0, "Min should be original value"
            assert max_val <= 200.0, "Max should be original value"
            
        finally:
            gs.run_command("g.remove", type="raster", name=output_name, flags="f", quiet=True)


@pytest.mark.parametrize("invalid_param", [
    {"memory": -1},  # Invalid memory
    {"method": "invalid_method"},  # Invalid interpolation method
])
def test_invalid_parameters(simple_raster, invalid_param):
    """Test that r.proj rejects invalid parameters."""
    output_name = "test_invalid_output"
    
    with pytest.raises(Exception):
        gs.run_command(
            "r.proj",
            input=simple_raster,
            output=output_name,
            location=gs.gisenv()["LOCATION_NAME"],
            mapset=gs.gisenv()["MAPSET"],
            **invalid_param,
            quiet=True
        )
