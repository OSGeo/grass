"""Pytest utilities for GRASS GIS testing

Provides comparison functions extracted from grass.gunittest for use with pytest.
"""

import grass.script as gs


def raster_fits_univar(map_name, reference, precision=1e-6):
    """Check if raster statistics match expected values.

    Args:
        map_name: Name of raster map to check
        reference: Dict of expected values (e.g., {"mean": 5.0, "min": 0})
        precision: Tolerance for floating point comparison

    Returns:
        (bool, str): (True, "") if match, (False, error_msg) if mismatch

    Example::

        matches, msg = raster_fits_univar("elevation", {"mean": 110.0})
        assert matches, msg
    """
    try:
        stats = gs.parse_command("r.univar", map=map_name, flags="g")
    except Exception as e:
        return False, f"Failed to get stats for '{map_name}': {e!s}"

    errors = []
    for key, expected in reference.items():
        if key not in stats:
            errors.append(f"'{key}' not in univar output")
            continue

        try:
            actual = float(stats[key])
            expected_val = float(expected)

            if abs(actual - expected_val) > precision:
                diff = abs(actual - expected_val)
                errors.append(
                    f"{key}: expected {expected_val}, got {actual} (diff: {diff:.2e})"
                )
        except (ValueError, TypeError) as e:
            errors.append(f"{key}: {e!s}")

    return (not errors, "\n".join(errors))
