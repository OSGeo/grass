import os
import subprocess
from pathlib import Path


def get_grass_config_path():
    grass_config_path = None
    try:
        grass_config_path = subprocess.run(
            ["grass", "--config", "path"], capture_output=True, text=True, check=True
        ).stdout.rstrip()
    except OSError:
        grass_config_path = None
    return grass_config_path


INITIAL_GISBASE = os.getenv("INITIAL_GISBASE", get_grass_config_path())
INITIAL_PWD = os.getenv("INITIAL_PWD", str(Path.cwd().absolute()))


def map_scripts_paths(old_path):
    if INITIAL_GISBASE is None or INITIAL_PWD is None:
        return old_path
    p = Path(old_path)
    extension = ".py"
    p_name = p.stem if p.suffix == extension else p.name
    temporal_base = Path(INITIAL_GISBASE) / "scripts" / "t.*"
    base = Path(INITIAL_GISBASE) / "scripts" / "*"
    if p.match(str(temporal_base)):
        return str(Path(INITIAL_PWD) / "temporal" / (p_name) / (p_name)) + extension
    if p.match(str(base)):
        return str(Path(INITIAL_PWD) / "scripts" / (p_name) / (p_name)) + extension

    return old_path


if __name__ == "__main__":
    from coverage import CoverageData

    a = CoverageData(".coverage")
    b = CoverageData(".coverage.fixed_scripts")
    b.update(a, map_path=map_scripts_paths)
    b.write()
