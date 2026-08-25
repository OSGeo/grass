"""Test t.rast.extract.

This test checks that t.rast.extract works also when
a) a fully qualified name is used in the expression,
b) it is run with a STRDS from another mapset as input and
c) the STRDS contains maps with identical temporal extent but with
   different semantic labels

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Stefan Blumentrath
"""

import os
import shutil
from pathlib import Path

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Pytest fixture to create a temporary GRASS project with a STRDS.

    This setup initializes the GRASS environment, sets the computational region,
    creates a series of raster maps with semantic labels, and rgisters
    the maps in a new SpaceTimeRasterDataSet (STRDS).

    Yields:
        session: active GRASS session with the prepared project and STRDS.

    """
    # Create a single temporary project directory once per module
    tmp_path = tmp_path_factory.mktemp("raster_time_series")

    # Remove if exists (defensive cleanup)
    if tmp_path.exists():
        shutil.rmtree(tmp_path)

    gs.create_project(tmp_path)

    with gs.setup.init(
        tmp_path.parent,
        location=tmp_path.name,
        env=os.environ.copy(),
    ) as session:
        tools = Tools(session=session)
        tools.g_mapset(mapset="perc", flags="c")
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        register_strings = []
        for i in range(1, 4):
            for label in ["A", "B"]:
                mapname = f"perc_{label}_{i}"
                semantic_label = f"S2{label}_{i}"
                tools.r_mapcalc(expression=f"{mapname} = {i}00", overwrite=True)
                tools.r_semantic_label(
                    map=mapname,
                    semantic_label=semantic_label,
                    overwrite=True,
                )
                register_strings.append(f"{mapname}|2025-08-0{i}|{semantic_label}\n")

        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="perc",
            title="A test",
            description="A test",
            overwrite=True,
        )
        tmp_file = tools.g_tempfile(pid=os.getpid()).text
        Path(tmp_file).write_text("".join(register_strings), encoding="UTF8")
        tools.t_register(
            type="raster",
            input="perc",
            file=tmp_file,
            overwrite=True,
        )
        yield session


def test_selection(session):
    """Perform a simple selection by datetime."""
    tools = Tools(session=session)
    tools.t_rast_extract(
        input="perc",
        output="perc_1",
        where="start_time > '2025-08-01'",
        verbose=True,
        overwrite=True,
    )


def test_selection_and_expression(session):
    """Perform a selection and a r.mapcalc expression with full name."""
    result = "perc_2"
    tools = Tools(session=session)
    tools.t_rast_extract(
        input="perc",
        output=result,
        where="start_time > '2025-08-01'",
        expression=" if(perc@perc>=200,perc@perc,null())",
        basename=result,
        nprocs=2,
        verbose=True,
        overwrite=True,
    )
    strds_info = tools.t_info(input=result, flags="g").keyval
    expected_info = {
        "start_time": "'2025-08-02 00:00:00'",
        "end_time": "'2025-08-03 00:00:00'",
        "name": result,
        "min_min": 200.0,
        "min_max": 300.0,
        "max_min": 200.0,
        "max_max": 300.0,
        "aggregation_type": "None",
        "number_of_semantic_labels": 4,
        "semantic_labels": "S2A_2,S2A_3,S2B_2,S2B_3",
        "number_of_maps": 4,
    }
    for k, v in expected_info.items():
        assert strds_info[k] == v, (
            f"Expected value for key '{k}' is {v}. Got: {strds_info[k]}"
        )


def test_inconsistent_selection_and_expression(session):
    """Perform a selection and a r.mapcalc expression with simple and full name."""
    result = "perc_3"
    tools = Tools(session=session)
    tools.t_rast_extract(
        input="perc",
        output=result,
        where="start_time > '2025-08-01'",
        expression=" if(perc>=200,perc@perc,null())",
        basename=result,
        nprocs=2,
        verbose=True,
        overwrite=True,
    )
    strds_info = tools.t_info(input=result, flags="g").keyval
    expected_info = {
        "start_time": "'2025-08-02 00:00:00'",
        "end_time": "'2025-08-03 00:00:00'",
        "name": result,
        "min_min": 200.0,
        "min_max": 300.0,
        "max_min": 200.0,
        "max_max": 300.0,
        "aggregation_type": "None",
        "number_of_semantic_labels": 4,
        "semantic_labels": "S2A_2,S2A_3,S2B_2,S2B_3",
        "number_of_maps": 4,
    }
    for k, v in expected_info.items():
        assert strds_info[k] == v, (
            f"Expected value for key '{k}' is {v}. Got: {strds_info[k]}"
        )


def test_selection_and_expression_simple_name(session):
    """Perform a selection and a r.mapcalc expression with simple name."""
    result = "perc_4"
    tools = Tools(session=session)
    tools.t_rast_extract(
        input="perc",
        output=result,
        where="start_time > '2025-08-01'",
        expression=" if(perc>=200,perc@perc,null())",
        basename=result,
        nprocs=2,
        verbose=True,
        overwrite=True,
    )
    strds_info = tools.t_info(input=result, flags="g").keyval
    expected_info = {
        "start_time": "'2025-08-02 00:00:00'",
        "end_time": "'2025-08-03 00:00:00'",
        "name": result,
        "min_min": 200.0,
        "min_max": 300.0,
        "max_min": 200.0,
        "max_max": 300.0,
        "aggregation_type": "None",
        "number_of_semantic_labels": 4,
        "semantic_labels": "S2A_2,S2A_3,S2B_2,S2B_3",
        "number_of_maps": 4,
    }
    for k, v in expected_info.items():
        assert strds_info[k] == v, (
            f"Expected value for key '{k}' is {v}. Got: {strds_info[k]}"
        )
