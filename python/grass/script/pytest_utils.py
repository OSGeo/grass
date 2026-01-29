"""Utility functions for pytest testing

This module provides comparison and assertion utilities extracted from
grass.gunittest to work with plain pytest assert statements.

These functions return (bool, str) tuples that can be used with pytest's
plain assert, enabling migration from gunittest to pytest.

Usage::

    from grass.script.pytest_utils import raster_fits_univar

    matches, msg = raster_fits_univar("test_map", {"mean": 5.0})
    assert matches, msg

Author: Saurabh Singh
Part of: GSoC 2025 pytest migration project
"""

import grass.script as gs


def raster_fits_univar(map_name, reference, precision=1e-6):
    """Check if raster statistics match expected values.

    Extracted from gunittest.case.TestCase.assertRasterFitsUnivar
    to work with pytest's plain assert statements.

    Args:
        map_name (str): Name of raster map to check
        reference (dict): Dictionary with expected values
                         (e.g., {"mean": 5.0, "min": 0, "max": 10})
        precision (float): Tolerance for floating point comparison

    Returns:
        tuple: (bool, str) - (matches, error_message)
               Returns (True, "") if all values match
               Returns (False, error_msg) if any value doesn't match

    Example::

        # Old gunittest way:
        self.assertRasterFitsUnivar("map", {"mean": 5.0})

        # New pytest way:
        matches, msg = raster_fits_univar("map", {"mean": 5.0})
        assert matches, msg
    """
    try:
        stats = gs.parse_command("r.univar", map=map_name, flags="g")
    except Exception as e:
        return False, f"Failed to get univar stats for '{map_name}': {e!s}"

    errors = []
    for key, expected in reference.items():
        if key not in stats:
            errors.append(f"Key '{key}' not found in univar output")
            continue

        try:
            actual = float(stats[key])
            expected_float = float(expected)

            if abs(actual - expected_float) > precision:
                errors.append(
                    f"{key}: expected {expected_float}, got {actual} "
                    f"(difference: {abs(actual - expected_float):.2e})"
                )
        except (ValueError, TypeError) as e:
            errors.append(f"{key}: conversion error - {e!s}")

    if errors:
        return False, "\n".join(errors)
    return True, ""


def raster_exists(map_name):
    """Check if raster map exists.

    Extracted from gunittest.case.TestCase.assertRasterExists

    Args:
        map_name (str): Name of raster map to check

    Returns:
        tuple: (bool, str) - (exists, error_message)
               Returns (True, "") if map exists
               Returns (False, error_msg) if map doesn't exist

    Example::

        # Old gunittest way:
        self.assertRasterExists("test_map")

        # New pytest way:
        exists, msg = raster_exists("test_map")
        assert exists, msg
    """
    result = gs.find_file(map_name, element="cell")
    if result["file"]:
        return True, ""
    return False, f"Raster map '{map_name}' does not exist"


def vector_exists(map_name):
    """Check if vector map exists.

    Extracted from gunittest.case.TestCase.assertVectorExists

    Args:
        map_name (str): Name of vector map to check

    Returns:
        tuple: (bool, str) - (exists, error_message)
               Returns (True, "") if map exists
               Returns (False, error_msg) if map doesn't exist

    Example::

        # Old gunittest way:
        self.assertVectorExists("test_vector")

        # New pytest way:
        exists, msg = vector_exists("test_vector")
        assert exists, msg
    """
    result = gs.find_file(map_name, element="vector")
    if result["file"]:
        return True, ""
    return False, f"Vector map '{map_name}' does not exist"


def module_succeeds(module, **kwargs):
    """Check if GRASS module runs successfully.

    Extracted from gunittest.case.TestCase.assertModule

    Args:
        module (str): Name of GRASS module to run
        **kwargs: Arguments to pass to the module

    Returns:
        tuple: (bool, str) - (success, error_message)
               Returns (True, "") if module succeeds
               Returns (False, error_msg) if module fails

    Example::

        # Old gunittest way:
        self.assertModule("r.mapcalc", expression="test = 1")

        # New pytest way:
        success, msg = module_succeeds("r.mapcalc", expression="test = 1")
        assert success, msg
    """
    try:
        gs.run_command(module, **kwargs)
        return True, ""
    except Exception as e:
        return False, f"Module '{module}' failed: {e!s}"
