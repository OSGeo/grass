# r.proj Unit Tests

This directory contains pytest-based unit tests for the `r.proj` GRASS GIS module.

## Structure

```
raster/r.proj/
├── tests/
│   ├── conftest.py                      # Pytest fixtures and configuration
│   ├── test_r_proj.py                   # Basic unit tests
│   ├── test_r_proj_comprehensive.py     # Comprehensive CRS transformation tests
│   └── README.md                        # This file
```

## Test Coverage

The test suite provides comprehensive coverage of r.proj functionality:

### Basic Tests (`test_r_proj.py`)

- **Identity transforms**: Same source/target CRS
- **Invalid location handling**: Graceful failure testing
- **Region preservation**: Computational region respect
- **Interpolation methods**: All methods (nearest, bilinear, bicubic)
- **Parameter validation**: Invalid parameter rejection

### Comprehensive CRS Transformation Tests (`test_r_proj_comprehensive.py`)

- **Actual CRS transformations**: UTM ↔ Lat/Lon reprojection
- **Datum transformations**: NAD83, WGS84
- **Print bounds flags**: Testing -p and -g flags (from manual examples)
- **Resolution parameter**: Resolution estimation and specification
- **All interpolation methods** across CRS boundaries
- **Memory parameter**: Cache size control and validation
- **Edge cases**: Nonexistent inputs, invalid methods
- **Region handling**: Output matching computational region

### Manual Example Coverage

Tests based on examples from the r.proj manual:

- Print bounds before reprojection (-p and -g flags)
- Resolution parameter usage
- Common transformations (projected ↔ geographic)

## Running the Tests

### Prerequisites

1. **GRASS GIS** installed and compiled
2. **pytest** installed: `pip install pytest`
3. **Active GRASS session** in a location with defined projection

### Basic Test Execution

```bash
# Start GRASS in any projected location
grass /path/to/grassdata/your_location/PERMANENT

# Navigate to the tests directory
cd raster/r.proj/tests

# Run all tests
pytest -v

# Run only basic tests
pytest test_r_proj.py -v

# Run only comprehensive CRS transformation tests
pytest test_r_proj_comprehensive.py -v

# Run specific test class
pytest test_r_proj_comprehensive.py::TestRProjCRSTransformations -v

# Run specific test
pytest test_r_proj_comprehensive.py::TestRProjCRSTransformations::test_utm_to_latlon_reprojection -v
```

### Test Locations

The comprehensive tests automatically create test locations with different CRS:

- **test_utm_nad83_z17n**: UTM Zone 17N (NAD83) - EPSG:26917
- **test_latlon_wgs84**: Lat/Lon WGS84 - EPSG:4326

These locations are created once and reused across test runs for efficiency. They can be manually removed from your GISDBASE if needed.

### Advanced Options

```bash
# Run with detailed output
pytest -v -s

# Run tests matching a pattern
pytest -k "interpolation" -v

# Run tests with a marker
pytest -m "requires_location" -v

# Show local variables on failure
pytest --showlocals

# Stop on first failure
pytest -x

# Run tests in parallel (requires pytest-xdist)
pytest -n auto

# Generate coverage report (requires pytest-cov)
pytest --cov=. --cov-report=html

# Run only fast tests (skip slow ones)
pytest -m "not slow"
```

### Debugging Failed Tests

```bash
# Run with Python debugger on failures
pytest --pdb

# Increase verbosity and show print statements
pytest -vv -s

# Show full diff on assertion failures
pytest -vv

# Keep test locations for manual inspection
# (Test locations are kept by default; clean them manually from GISDBASE)
```

## Writing New Tests

### Test Function Naming

- Test files: `test_*.py`
- Test functions: `test_*`
- Test classes: `Test*`

### Using Fixtures

The conftest.py provides several useful fixtures:

```python
def test_example(simple_raster, temp_region, preserve_location):
    """Example test using fixtures."""
    # simple_raster provides a test raster
    # temp_region ensures region is restored after test
    # preserve_location ensures we return to original location
    gs.run_command("r.proj", input=simple_raster, ...)
```

### Parametrized Tests

Use `@pytest.mark.parametrize` for testing multiple inputs:

```python
@pytest.mark.parametrize("method", ["nearest", "bilinear", "bicubic"])
def test_methods(simple_raster, method):
    gs.run_command("r.proj", input=simple_raster, method=method, ...)
```

### Testing Across Locations

When testing actual CRS transformations, tests need to:

1. Create or use test locations with different CRS
2. Switch between locations during the test
3. Clean up properly to avoid state leakage

Example pattern:
```python
def test_cross_crs_reprojection(utm_location, latlon_location):
    current_env = gs.gisenv()
    
    # Switch to target location
    gs.run_command("g.mapset", location=latlon_location, ...)
    
    try:
        # Run r.proj from source to target
        gs.run_command("r.proj", location=utm_location, ...)
        # Verify results
    finally:
        # Always restore original location
        gs.run_command("g.mapset", location=current_env["LOCATION_NAME"], ...)
```

## Test Design Philosophy

### Why Two Test Files?

1. **test_r_proj.py** - Fast, simple tests
   - Run quickly in any single location
   - Good for basic functionality and regression testing
   - Don't require location switching

2. **test_r_proj_comprehensive.py** - Complete coverage
   - Test actual CRS transformations
   - Create temporary locations as needed
   - Cover real-world use cases from the manual
   - May be slower but provide confidence in PROJ integration

### Relationship to Issue #2512

This test suite addresses the requirements in issue #2512:

✅ **Basic unit tests with artificial data**
- Simple rasters with known values
- Predictable, deterministic results

✅ **Different CRS transformations**

- UTM ↔ Lat/Lon (most common transformation)
- Datum transformations (NAD83 vs WGS84)
- Extensible to other CRS combinations

✅ **Examples from manual**
- Print bounds flags (-p, -g)
- Resolution parameter usage
- Memory parameter testing

✅ **pytest-based** (modern testing framework)
- Replaces older gunittest approach
- Better integration with CI/CD
- More Pythonic and maintainable

### Future Extensions

Potential areas for expansion:
- **More CRS combinations**: State Plane, different UTM zones, international CRS
- **Performance benchmarks**: Large raster handling, OpenMP testing
- **Null value handling**: Nodata regions, masked areas
- **Extreme resolutions**: Very fine and very coarse target resolutions
- **Edge case geometries**: Polar regions, dateline crossing, world edges

## References

- [GRASS Testing Documentation](https://grass.osgeo.org/grass-devel/manuals/libpython/gunittest_testing.html)
- [pytest Documentation](https://docs.pytest.org/)
- [r.proj Manual](https://grass.osgeo.org/grass-devel/manuals/r.proj.html)
- [GitHub Issue #2512](https://github.com/OSGeo/grass/issues/2512)

## Contributing

When adding tests:

1. Follow existing naming conventions
2. Add docstrings explaining what each test validates
3. Use fixtures for setup/teardown
4. Keep tests independent (no reliance on test execution order)
5. Clean up created maps and locations in teardown
6. Reference manual examples when applicable
7. Consider both normal cases and edge cases

For questions or to report issues with these tests:

- GitHub: <https://github.com/OSGeo/grass/issues/2512>
- Mailing list: <grass-dev@lists.osgeo.org>
