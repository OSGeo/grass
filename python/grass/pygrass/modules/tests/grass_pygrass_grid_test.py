"""Test main functions of PyGRASS GridModule"""

import multiprocessing

import pytest

import grass.script as gs
import grass.script.setup as grass_setup


def max_processes():
    """Get max useful number of parallel processes to run"""
    return min(multiprocessing.cpu_count(), 4)


# GridModule uses C libraries which can easily initialize only once
# and thus can't easily change location/mapset, so we use a subprocess
# to separate individual GridModule calls.
def run_in_subprocess(function):
    """Run function in a separate process"""
    process = multiprocessing.Process(target=function)
    process.start()
    process.join()


@pytest.mark.parametrize("processes", list(range(1, max_processes() + 1)) + [None])
def test_processes(tmp_path, processes):
    """Check that running with multiple processes works"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)

        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        def run_grid_module():
            # modules/shortcuts calls get_commands which requires GISBASE.
            # pylint: disable=import-outside-toplevel
            from grass.pygrass.modules.grid import GridModule

            grid = GridModule(
                "r.slope.aspect",
                width=10,
                height=5,
                overlap=2,
                processes=processes,
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
            grid.run()

        run_in_subprocess(run_grid_module)

        info = gs.raster_info("slope")
        assert info["min"] > 0


# @pytest.mark.parametrize("split", [False])  # True does not work.


@pytest.mark.parametrize("width", [5, 10, 50])  # None does not work.
@pytest.mark.parametrize("height", [5, 10, 50])
def test_tiling_schemes(tmp_path, width, height):
    """Check that different shapes of tiles work"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)

        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        def run_grid_module():
            # modules/shortcuts calls get_commands which requires GISBASE.
            # pylint: disable=import-outside-toplevel
            from grass.pygrass.modules.grid import GridModule

            grid = GridModule(
                "r.slope.aspect",
                width=width,
                height=height,
                overlap=2,
                processes=max_processes(),
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
            grid.run()

        run_in_subprocess(run_grid_module)

        info = gs.raster_info("slope")
        assert info["min"] > 0


@pytest.mark.parametrize("overlap", [0, 1, 2, 5])
def test_overlaps(tmp_path, overlap):
    """Check that overlap accepts different values"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)
        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        def run_grid_module():
            # modules/shortcuts calls get_commands which requires GISBASE.
            # pylint: disable=import-outside-toplevel
            from grass.pygrass.modules.grid import GridModule

            grid = GridModule(
                "r.slope.aspect",
                width=10,
                height=5,
                overlap=overlap,
                processes=max_processes(),
                elevation=surface,
                slope="slope",
                aspect="aspect",
            )
            grid.run()

        run_in_subprocess(run_grid_module)

        info = gs.raster_info("slope")
        assert info["min"] > 0


@pytest.mark.parametrize("clean", [True, False])
def test_cleans(tmp_path, clean):
    """Check that temporary mapsets are cleaned when appropriate"""
    location = "test"
    mapset_prefix = "abc"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.region", s=0, n=50, w=0, e=50, res=1)
        surface = "surface"
        gs.run_command("r.surf.fractal", output=surface)

        def run_grid_module():
            # modules/shortcuts calls get_commands which requires GISBASE.
            # pylint: disable=import-outside-toplevel
            from grass.pygrass.modules.grid import GridModule

            grid = GridModule(
                "r.slope.aspect",
                width=10,
                height=5,
                overlap=0,
                processes=max_processes(),
                elevation=surface,
                slope="slope",
                aspect="aspect",
                mapset_prefix=mapset_prefix,
            )
            grid.run(clean=clean)

        run_in_subprocess(run_grid_module)

        path = tmp_path / location
        prefixed = 0
        for item in path.iterdir():
            if item.is_dir():
                if clean:
                    # We know right away something is wrong.
                    assert not item.name.startswith(mapset_prefix), "Mapset not cleaned"
                else:
                    # We need to see if there is at least one prefixed mapset.
                    prefixed += int(item.name.startswith(mapset_prefix))
        if not clean:
            assert prefixed, "Not even one prefixed mapset"
