# SPDX-License-Identifier: GPL-2.0-or-later

"""Test v.proj coordinate correctness.

With smax=0 (no densification) v.proj transforms all vertices of a line
in one GPJ_transform_array() call, while with densification enabled each
vertex goes through a separate GPJ_transform() call. Both paths must
produce the same coordinates.
"""

import pytest

from grass.tools import Tools


def read_line_coords(tools, vector_map):
    """Return list of (x, y) vertex tuples of the single line in the map"""
    ascii_out = tools.v_out_ascii(input=vector_map, format="standard", type="line")
    coords = []
    for line in ascii_out.text.splitlines():
        parts = line.split()
        if len(parts) == 2 and "." in parts[0]:
            coords.append((float(parts[0]), float(parts[1])))
    return coords


def test_smax_zero_matches_per_vertex_path(reprojection_session):
    """All vertices must be reprojected, not just the first one"""
    tools = Tools(session=reprojection_session.session)
    tools.v_proj(
        project=reprojection_session.source, input="line", output="batch", smax=0
    )
    tools.v_proj(project=reprojection_session.source, input="line", output="per_vertex")
    batch = read_line_coords(tools, "batch")
    per_vertex = read_line_coords(tools, "per_vertex")
    assert len(batch) == 3
    assert batch == pytest.approx(per_vertex)
