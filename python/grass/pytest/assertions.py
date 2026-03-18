import grass.script as gs


def raster_exists(name, env=None):
    """Assert that a raster map exists in the given mapset."""
    result = gs.find_file(name, element="cell", env=env)
    
    if not result["name"]:
        raise AssertionError(f"Raster map '{name}' not found.")
