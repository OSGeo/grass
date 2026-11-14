# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for the sampling methods of sample_by_dataset().

Each map lists the methods which select it. Interval maps are named after
their relation to the granule as defined by TemporalExtent. The code reads the
relations of the granule, so its "overlaps" selects the map "overlapped".
"follows" and "precedes" match the t.sample manual examples, not the docstring.
"""

import os

import pytest

import grass.script as gs
import grass.temporal as tgis
from grass.tools import Tools

METHODS = ("start", "contain", "during", "overlap", "equal", "follows", "precedes")
MAPS = {
    "equal": ((10, 20), {"start", "equal"}),
    "during": ((12, 18), {"start", "contain"}),
    "contains": ((5, 25), {"during"}),
    "starts": ((10, 15), {"start", "contain"}),
    "started": ((10, 25), {"start", "during"}),
    "finishes": ((15, 20), {"start", "contain"}),
    "finished": ((5, 20), {"during"}),
    "overlaps": ((5, 15), {"overlap"}),
    "overlapped": ((15, 25), {"start", "overlap"}),
    "precedes": ((1, 10), {"follows"}),
    "follows": ((20, 30), {"precedes"}),
    "before": ((1, 5), set()),
    "after": ((25, 30), set()),
    "instance_before": ((5, None), set()),
    "instance_at_start": ((10, None), {"start", "contain"}),
    "instance_inside": ((12, None), {"start", "contain"}),
    "instance_at_end": ((20, None), {"precedes"}),
    "instance_after": ((25, None), set()),
}


@pytest.fixture(scope="module")
def selected(tmp_path_factory):
    """Names of the maps which each method selects for the granule."""
    project = tmp_path_factory.mktemp("data") / "project"
    gs.create_project(project)
    with (
        pytest.MonkeyPatch.context() as monkeypatch,
        gs.setup.init(project, env=os.environ.copy()) as session,
    ):
        for key, value in session.env.items():
            if os.environ.get(key) != value:
                monkeypatch.setenv(key, value)
        tools = Tools(session=session)
        tgis.init()
        datasets = {
            "sampler": {"granule": (10, 20)},
            "sampled": {name: times for name, (times, _) in MAPS.items()},
        }
        for dataset, maps in datasets.items():
            tgis.open_new_stds(dataset, "strds", "absolute", dataset, dataset, "mean")
            for name, (start, end) in maps.items():
                tools.r_mapcalc(expression=f"{name} = 1")
                tgis.register_maps_in_space_time_dataset(
                    "raster",
                    dataset,
                    maps=name,
                    start=f"2001-01-{start:02d}",
                    end=f"2001-01-{end:02d}" if end else None,
                )
        sampled = tgis.open_old_stds("sampled", "strds")
        sampler = tgis.open_old_stds("sampler", "strds")
        selection = {}
        for method in METHODS:
            (entry,) = sampled.sample_by_dataset(stds=sampler, method=[method])
            selection[method] = {map_.get_name() for map_ in entry["samples"]}
        yield selection


@pytest.mark.parametrize("name", MAPS)
def test_sampling_methods(selected, name):
    """Check which sampling methods select the map for the granule."""
    methods = {method for method in METHODS if name in selected[method]}
    assert methods == MAPS[name][1]
