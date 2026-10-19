"""Parsing of semantic-label band references in STDS names.

Regression tests for https://github.com/OSGeo/grass/issues/7670: a dataset
name of the form ``name.semantic_label`` (e.g. ``ndvi.S2_8``) must have the
label split off for raster tools too, not only when the element type is
``strds``. Creating a new dataset with a dot in the name stays an error.
"""

import os

import pytest

import grass.script as gs
from grass.exceptions import FatalError


@pytest.fixture(scope="module")
def tgis_session(tmp_path_factory):
    """A minimal initialized temporal session in a fresh project."""
    tmp_path = tmp_path_factory.mktemp("semantic_label_temporal")
    project = tmp_path / "test_project"
    gs.create_project(project)

    with (
        pytest.MonkeyPatch.context() as monkeypatch,
        gs.setup.init(project, env=os.environ.copy()) as session,
    ):
        for key, value in session.env.items():
            if os.environ.get(key) != value:
                monkeypatch.setenv(key, value)

        import grass.temporal as tgis

        tgis.init()
        yield session


@pytest.mark.parametrize(
    ("ident", "stds_type", "expected"),
    [
        # No dot, no mapset: name is returned unchanged.
        ("elevation", "strds", ("elevation", None, None)),
        # Fully qualified name: mapset is split off (feature from #7670).
        ("test@PERMANENT", "strds", ("test", "PERMANENT", None)),
        # Band reference on an strds.
        ("ndvi.S2_8", "strds", ("ndvi", None, "S2_8")),
        # Band reference reached with the raster element type (the regression:
        # t.rast.mapcalc calls open_old_stds with "raster", not "strds").
        ("ndvi.S2_8", "raster", ("ndvi", None, "S2_8")),
        ("ndvi.S2_8", "rast", ("ndvi", None, "S2_8")),
        # Band reference plus a fully qualified mapset.
        ("ndvi.S2_8@PERMANENT", "raster", ("ndvi", "PERMANENT", "S2_8")),
        # 3D raster and vector STDS have no semantic labels: the dot stays.
        ("ndvi.S2_8", "str3ds", ("ndvi.S2_8", None, None)),
        ("ndvi.S2_8", "stvds", ("ndvi.S2_8", None, None)),
    ],
)
def test_parse_id(tgis_session, ident, stds_type, expected):
    """_parse_id splits the band reference for raster STDS types only."""
    from grass.temporal.open_stds import _parse_id

    assert _parse_id(ident, stds_type=stds_type) == expected


def test_parse_id_rejects_double_dot(tgis_session):
    """A semantic label cannot itself contain a dot."""
    from grass.temporal.open_stds import _parse_id

    with pytest.raises((SystemExit, FatalError)):
        _parse_id("ndvi.S2_8.extra", stds_type="raster")


def test_check_new_stds_rejects_dotted_name(tgis_session):
    """Creating a dataset whose name contains a dot is an error (8.4 behavior)."""
    from grass.temporal.open_stds import check_new_stds

    with pytest.raises((SystemExit, FatalError)):
        check_new_stds("my.dataset", "strds")


def test_check_new_stds_accepts_plain_name(tgis_session):
    """A plain new-dataset name is not rejected by the dot guard."""
    from grass.temporal.open_stds import check_new_stds

    sp = check_new_stds("newdataset", "strds")
    assert sp is not None
