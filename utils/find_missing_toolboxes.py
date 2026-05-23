#!/usr/bin/env python3

"""
find_missing_toolboxes.py  --  Report tools present in the source tree
but absent from the GUI toolbox configuration.

A directory is treated as a tool when its name matches the GRASS naming
convention (prefix.name) and it contains either a Python entry point
(<name>.py) or a C entry point (main.c).

Tools already accessible in the GUI are collected from three sources:
  - <module-item> entries in toolboxes.xml
  - <related-module> entries in wxgui_items.xml
  - <command> entries in wxgui_items.xml

Usage:

python utils/find_missing_toolboxes.py

@author
"""

import re
import sys
from pathlib import Path
from xml.etree import ElementTree as ET

# ---------------------------------------------------------------------------
# Tools intentionally absent from the GUI menu.
# d.*  Display tools are driven from the map display, not the module menu.
# g.*  Internal plumbing tools not meant for direct user interaction.
# ---------------------------------------------------------------------------
EXCLUDED = {
    "d.background",
    "d.barscale",
    "d.colorlist",
    "d.colortable",
    "d.correlate",
    "d.erase",
    "d.extract",
    "d.font",
    "d.fontlist",
    "d.frame",
    "d.geodesic",
    "d.graph",
    "d.grid",
    "d.his",
    "d.histogram",
    "d.info",
    "d.labels",
    "d.legend",
    "d.legend.vect",
    "d.linegraph",
    "d.mon",
    "d.northarrow",
    "d.out.file",
    "d.path",
    "d.polar",
    "d.profile",
    "d.rast",
    "d.rast.arrow",
    "d.rast.leg",
    "d.rast.num",
    "d.redraw",
    "d.rgb",
    "d.rhumbline",
    "d.shade",
    "d.text",
    "d.title",
    "d.to.rast",
    "d.vect",
    "d.vect.chart",
    "d.vect.thematic",
    "d.what.rast",
    "d.what.vect",
    "d.where",
    "g.cairocomp",
    "g.dirseps",
    "g.filename",
    "g.findetc",
    "g.findfile",
    "g.gui",
    "g.html2man",
    "g.message",
    "g.mkfontcap",
    "g.parser",
    "g.pnmcomp",
    "g.ppmtopng",
    "g.search.modules",
    "g.setproj",
    "g.tempfile",
    "g.version",
}

TOOL_PATTERN = re.compile(r"^[a-z][a-z0-9]*\.[a-z][a-z0-9._]*$")


def find_source_tools(root: Path) -> set[str]:
    """Return names of all tools found in the source tree.

    A directory qualifies when its name matches the GRASS tool naming
    convention and it contains a Python entry point or a C main.c.
    Searches up to three directory levels deep to catch tools nested
    inside parent directories (e.g. raster/r.sim/r.sim.water).
    """
    tools = set()
    for depth in range(2, 5):  # mindepth 2, maxdepth 4
        pattern = "/".join(["*"] * depth)
        for candidate in root.glob(pattern):
            if not candidate.is_dir():
                continue
            if candidate.parts[len(root.parts)] == "doc":
                continue
            if not TOOL_PATTERN.match(candidate.name):
                continue
            if (candidate / f"{candidate.name}.py").exists() or (
                candidate / "main.c"
            ).exists():
                tools.add(candidate.name)
    return tools


def find_registered_tools(toolboxes: Path, wxgui_items: Path) -> set[str]:
    """Return tool names already accessible somewhere in the GUI."""
    registered = set()

    tree = ET.parse(toolboxes)
    for el in tree.iter("module-item"):
        name = el.get("name")
        if name:
            registered.add(name)

    tree = ET.parse(wxgui_items)
    for el in tree.iter("related-module"):
        if el.text:
            registered.add(el.text.strip())
    for el in tree.iter("command"):
        if el.text:
            # Extract only the tool name (first word before any flags).
            tool = el.text.strip().split()[0]
            if TOOL_PATTERN.match(tool):
                registered.add(tool)

    return registered


def main() -> int:
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path.cwd()

    toolboxes = root / "gui" / "wxpython" / "xml" / "toolboxes.xml"
    wxgui_items = root / "gui" / "wxpython" / "xml" / "wxgui_items.xml"

    for path in (toolboxes, wxgui_items):
        if not path.exists():
            print(
                f"ERROR: {path} not found. Run from the repository root.",
                file=sys.stderr,
            )
            return 1

    source_tools = find_source_tools(root)
    registered = find_registered_tools(toolboxes, wxgui_items)

    missing = sorted(source_tools - registered - EXCLUDED)

    if not missing:
        print("All source tools are registered in toolboxes.xml.")
        return 0

    print(f"Tools in source tree but missing from toolboxes.xml ({len(missing)}):\n")
    for name in missing:
        print(f"  {name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
