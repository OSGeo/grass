"""Tests for category values read from the standard format

Vect_cat_set() has its range check commented out, so a category read from an
ASCII file was stored whatever its value. A negative one survived the import
and only caused trouble later: v.out.ogr skipped such a point without -c and
exported it twice with it (https://github.com/OSGeo/grass/issues/6563).
"""

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError

VALID = "P 1 1\n0 0\n1 1\nP 1 1\n1 0\n1 2\n"
NEGATIVE_CATEGORY = "P 1 1\n0 0\n1 1\nP 1 1\n1 0\n1 -2147483647\n"
ZERO_LAYER = "P 1 1\n0 0\n0 5\n"
ZERO_CATEGORY = "P 1 1\n0 0\n1 0\n"


def import_ascii(session, tmp_path, text, name):
    path = tmp_path / f"{name}.txt"
    path.write_text(text)
    gs.run_command(
        "v.in.ascii",
        input=str(path),
        output=name,
        format="standard",
        flags="n",
        overwrite=True,
        env=session.env,
    )


def test_valid_categories_are_imported(xy_session, tmp_path):
    """A file with ordinary categories still imports"""
    import_ascii(xy_session, tmp_path, VALID, "valid")
    categories = gs.read_command(
        "v.category", input="valid", option="print", env=xy_session.env
    )
    assert categories.split() == ["1", "2"]


def test_zero_category_is_accepted(xy_session, tmp_path):
    """Category 0 is allowed, since OGR layers use it"""
    import_ascii(xy_session, tmp_path, ZERO_CATEGORY, "zerocat")
    categories = gs.read_command(
        "v.category", input="zerocat", option="print", env=xy_session.env
    )
    assert categories.split() == ["0"]


def test_negative_category_is_rejected(xy_session, tmp_path):
    """A negative category fails the import instead of being stored"""
    with pytest.raises(CalledModuleError):
        import_ascii(xy_session, tmp_path, NEGATIVE_CATEGORY, "negcat")


def test_zero_layer_is_rejected(xy_session, tmp_path):
    """Layer numbers start at 1, so 0 fails the import"""
    with pytest.raises(CalledModuleError):
        import_ascii(xy_session, tmp_path, ZERO_LAYER, "zerolayer")
