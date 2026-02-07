"""Test t.vect.list output formats"""

import pytest

import grass.script as gs

yaml = pytest.importorskip("yaml", reason="PyYAML package not available")


@pytest.mark.needs_solo_run
def test_defaults(space_time_vector_dataset):
    """Check that the module runs with default parameters"""
    gs.run_command(
        "t.vect.list",
        input=space_time_vector_dataset.name,
        env=space_time_vector_dataset.session.env,
    )
