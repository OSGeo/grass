# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of text rendering with FreeType fonts in the display drivers"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools

WIDTH = 200
HEIGHT = 450


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Session in an XY project with a region filling the whole image"""
    project = tmp_path_factory.mktemp("lib_driver") / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=HEIGHT, w=0, e=WIDTH, res=1)
        yield session


def render_tools(session, driver, output):
    """Return a Tools object which renders into the given PPM file.

    A copy of the session environment is used so the render settings do not
    leak into the shared session fixture.
    """
    env = dict(session.env)
    env["GRASS_RENDER_IMMEDIATE"] = driver
    env["GRASS_RENDER_FILE"] = str(output)
    env["GRASS_RENDER_WIDTH"] = str(WIDTH)
    env["GRASS_RENDER_HEIGHT"] = str(HEIGHT)
    return Tools(env=env, consistent_return_value=True)


@pytest.fixture(scope="module")
def freetype_font(session, tmp_path_factory):
    """Name of a FreeType font from the font configuration file

    DejaVu Sans is preferred, then any other sans font. The tests are skipped
    when the font configuration file is missing or lists no FreeType font.
    """
    output = tmp_path_factory.mktemp("d_fontlist") / "fontlist.ppm"
    # Without a font configuration file d.fontlist prints nothing.
    fonts = render_tools(session, "png", output).d_fontlist(flags="v").text or ""
    # Each line is name|longname|type|path|index|encoding| and type 1 is
    # a FreeType font.
    names = [
        fields[0]
        for fields in (line.split("|") for line in fonts.splitlines())
        if len(fields) > 2 and fields[2] == "1"
    ]
    if not names:
        pytest.skip("No FreeType font in the font configuration file")
    if "DejaVuSans" in names:
        return "DejaVuSans"
    sans = [name for name in names if "sans" in name.lower()]
    return sans[0] if sans else names[0]


def rows_with_text(path):
    """Return the indices of the image rows which have a dark pixel"""
    header = f"P6\n{WIDTH} {HEIGHT}\n255\n".encode()
    data = path.read_bytes()
    assert data.startswith(header)
    pixels = data[len(header) :]
    row_size = 3 * WIDTH
    return [
        row
        for row in range(HEIGHT)
        if min(pixels[row * row_size : (row + 1) * row_size]) < 128
    ]


@pytest.mark.parametrize("driver", ["png", "cairo"])
@pytest.mark.parametrize("position", [10, 90])
def test_text_drawn_at_position(session, freetype_font, tmp_path, driver, position):
    """Text is drawn at its position, both low and high in the image.

    FreeType 2.14.2 refuses to render glyphs far from the origin, which is the
    lower left corner of the image, so text high up in the image was missing.
    The text is about 20 pixels tall and has no descenders, so it should lie
    in the 50 pixels above its baseline, allowing a few pixels below it.
    """
    output = tmp_path / "text.ppm"
    tools = render_tools(session, driver, output)
    tools.d_text(
        text="GRASS 123",
        at=(5, position),
        size=5,
        color="black",
        font=freetype_font,
    )
    rows = rows_with_text(output)
    assert rows, "No text drawn"
    baseline = HEIGHT * (100 - position) / 100
    assert baseline - 50 < min(rows)
    assert max(rows) < baseline + 5
