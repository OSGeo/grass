"""Pytest configuration and fixtures for GRASS GIS tests.

This conftest.py provides common fixtures and setup for r.proj testing.
"""

import pytest
import grass.script as gs
import os


@pytest.fixture(scope="session")
def grass_session():
    """
    Fixture to ensure GRASS GIS session is properly initialized.
    
    This is a session-scoped fixture that runs once for all tests.
    GRASS should already be running when pytest is executed.
    """
    # Verify GRASS is running
    try:
        gisenv = gs.gisenv()
        print(f"\nGRASS Session Info:")
        print(f"  GISDBASE: {gisenv.get('GISDBASE')}")
        print(f"  LOCATION: {gisenv.get('LOCATION_NAME')}")
        print(f"  MAPSET: {gisenv.get('MAPSET')}")
    except Exception as e:
        pytest.fail(f"GRASS GIS session not available: {e}")
    
    yield gisenv
    
    # Session cleanup if needed
    print("\nGRASS session fixture teardown complete")


@pytest.fixture(scope="function")
def temp_region():
    """
    Fixture to save and restore the computational region for each test.
    
    This ensures tests don't interfere with each other by modifying
    the global computational region.
    """
    # Save current region
    temp_region_name = f"test_region_{os.getpid()}"
    gs.run_command("g.region", save=temp_region_name, quiet=True)
    
    yield
    
    # Restore region
    gs.run_command("g.region", region=temp_region_name, quiet=True)
    gs.run_command("g.remove", type="region", name=temp_region_name, flags="f", quiet=True)


@pytest.fixture(scope="function")
def preserve_location():
    """
    Fixture to preserve and restore current location/mapset.
    
    Some tests need to switch between locations. This fixture ensures
    they return to the original location after completion.
    """
    original_gisenv = gs.gisenv()
    original_location = original_gisenv["LOCATION_NAME"]
    original_mapset = original_gisenv["MAPSET"]
    original_gisdbase = original_gisenv["GISDBASE"]
    
    yield {
        "location": original_location,
        "mapset": original_mapset,
        "gisdbase": original_gisdbase
    }
    
    # Restore original location/mapset
    try:
        gs.run_command(
            "g.mapset",
            mapset=original_mapset,
            location=original_location,
            dbase=original_gisdbase,
            quiet=True
        )
    except Exception:
        # If restoration fails, at least try to get back to a valid state
        pass


@pytest.fixture(autouse=True)
def cleanup_test_maps():
    """
    Auto-use fixture to clean up any test maps after each test.
    
    This runs automatically after every test to ensure cleanup.
    """
    yield
    
    # After test: remove any maps starting with "test_"
    try:
        # Get list of all raster maps starting with "test_"
        rasters = gs.list_grouped(type="raster", pattern="test_*")
        current_mapset = gs.gisenv()["MAPSET"]
        
        if current_mapset in rasters and rasters[current_mapset]:
            test_maps = rasters[current_mapset]
            if test_maps:
                gs.run_command(
                    "g.remove",
                    type="raster",
                    name=test_maps,
                    flags="f",
                    quiet=True
                )
    except Exception:
        # Cleanup is best-effort
        pass


def pytest_configure(config):
    """Configure pytest with GRASS-specific settings."""
    config.addinivalue_line(
        "markers",
        "slow: marks tests as slow (deselect with '-m \"not slow\"')"
    )
    config.addinivalue_line(
        "markers",
        "requires_location: marks tests that require specific GRASS location"
    )
