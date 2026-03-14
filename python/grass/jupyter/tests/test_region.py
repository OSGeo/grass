import os
import subprocess
import sys
import textwrap

import pytest


def run_in_subprocess(file):
    """Run script in a separate process to avoid polluting the global environment."""
    env = os.environ.copy()
    env["PYTHONPATH"] = os.pathsep.join(sys.path)

    process = subprocess.run(
        [sys.executable, os.fspath(file)],
        capture_output=True,
        text=True,
        env=env,
    )

    error_msg = (
        f"Subprocess script failed.\nStderr: {process.stderr}\nStdout: {process.stdout}"
    )
    assert process.returncode == 0, error_msg

    return process.stdout


@pytest.mark.integration
def test_get_region_returns_dict(tmp_path):
    """Check that get_region successfully connects and returns a dictionary."""
    project = tmp_path / "test_dict_project"

    script = f"""
    import grass.script as gs
    import grass.jupyter as gj
    from grass.jupyter.utils import get_region

    gs.create_project(r"{project}")
    session = gj.init(r"{project}")
    try:
        region_data = get_region()
        assert isinstance(region_data, dict), "get_region did not return a dict"
    finally:
        session.finish()
    """
    file = tmp_path / "test_returns_dict.py"
    file.write_text(textwrap.dedent(script))
    run_in_subprocess(file)


@pytest.mark.integration
def test_get_region_has_keys(tmp_path):
    """Check that the returned dictionary contains the expected geographic keys."""
    project = tmp_path / "test_keys_project"

    script = f"""
    import grass.script as gs
    import grass.jupyter as gj
    from grass.jupyter.utils import get_region

    gs.create_project(r"{project}")
    session = gj.init(r"{project}")
    try:
        region_data = get_region()
        expected_keys = ["n", "s", "e", "w", "nsres", "ewres"]

        for key in expected_keys:
            assert key in region_data, f"Missing expected key: {{key}}"
    finally:
        session.finish()
    """
    file = tmp_path / "test_has_keys.py"
    file.write_text(textwrap.dedent(script))
    run_in_subprocess(file)


@pytest.mark.integration
def test_get_region_values_are_numeric(tmp_path):
    """Check that all values in the returned dictionary are numeric."""
    project = tmp_path / "test_numeric_project"

    script = f"""
    import grass.script as gs
    import grass.jupyter as gj
    from grass.jupyter.utils import get_region

    gs.create_project(r"{project}")
    session = gj.init(r"{project}")
    try:
        region_data = get_region()
        non_numeric = {{k: v for k, v in region_data.items() if not isinstance(v, (int, float))}}
        assert not non_numeric, f"Non-numeric values found: {{non_numeric}}"
    finally:
        session.finish()
    """
    file = tmp_path / "test_numeric_values.py"
    file.write_text(textwrap.dedent(script))
    run_in_subprocess(file)


@pytest.mark.integration
def test_get_region_geographic_sanity(tmp_path):
    """Check that north > south, east > west, and resolutions are positive."""
    project = tmp_path / "test_sanity_project"

    script = f"""
    import grass.script as gs
    import grass.jupyter as gj
    from grass.jupyter.utils import get_region

    gs.create_project(r"{project}")
    session = gj.init(r"{project}")
    try:
        r = get_region()
        assert r["n"] > r["s"], f"n={{r['n']}} is not > s={{r['s']}}"
        assert r["e"] > r["w"], f"e={{r['e']}} is not > w={{r['w']}}"
        assert r["nsres"] > 0, f"nsres={{r['nsres']}} is not positive"
        assert r["ewres"] > 0, f"ewres={{r['ewres']}} is not positive"
    finally:
        session.finish()
    """
    file = tmp_path / "test_geo_sanity.py"
    file.write_text(textwrap.dedent(script))
    run_in_subprocess(file)


@pytest.mark.integration
def test_get_region_reflects_changes(tmp_path):
    """Verify get_region reads the live state after the region is modified."""
    project = tmp_path / "test_reflects_project"

    script = f"""
    import grass.script as gs
    import grass.jupyter as gj
    from grass.jupyter.utils import get_region

    gs.create_project(r"{project}")
    session = gj.init(r"{project}")
    try:
        gs.run_command("g.region", n=60, s=50, e=30, w=10, res=1)

        r = get_region()

        assert abs(r["n"] - 60) < 1e-6, f"Expected n=60, got {{r['n']}}"
        assert abs(r["s"] - 50) < 1e-6, f"Expected s=50, got {{r['s']}}"
        assert abs(r["e"] - 30) < 1e-6, f"Expected e=30, got {{r['e']}}"
        assert abs(r["w"] - 10) < 1e-6, f"Expected w=10, got {{r['w']}}"
    finally:
        session.finish()
    """
    file = tmp_path / "test_reflects_changes.py"
    file.write_text(textwrap.dedent(script))
    run_in_subprocess(file)


@pytest.mark.integration
def test_get_region_is_read_only(tmp_path):
    """Check that calling get_region() does not modify the active GRASS region."""
    project = tmp_path / "test_readonly_project"

    script = f"""
    import grass.script as gs
    import grass.jupyter as gj
    from grass.jupyter.utils import get_region

    gs.create_project(r"{project}")
    session = gj.init(r"{project}")
    try:
        before = gs.region()
        get_region()
        after = gs.region()

        for key in ("n", "s", "e", "w", "nsres", "ewres"):
            assert before[key] == after[key], (
                f"get_region() changed '{{key}}': {{before[key]}} -> {{after[key]}}"
            )
    finally:
        session.finish()
    """
    file = tmp_path / "test_read_only.py"
    file.write_text(textwrap.dedent(script))
    run_in_subprocess(file)
