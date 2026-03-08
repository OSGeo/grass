import grass.script as gs


def rasterExists(map_name, env=None):
    """Check if a raster map exists in the given mapset."""
    result = gs.find_file(map_name, element="cell", env=env)
    return bool(result["name"])