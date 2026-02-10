#
# AUTHOR(S): Saurabh Singh
#
# PURPOSE:   Legend support for interactive maps in Jupyter Notebooks
#
# COPYRIGHT: (C) 2025 by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Legend generation for raster layers in interactive maps"""

import grass.script as gs


def parse_colors(mapname):
    """Parse color table from a raster map using r.colors.out.

    :param str mapname: Name of the raster map
    :return: Dictionary containing color information with keys:
             - type: "categorical" or "continuous"
             - items: list of dicts with value, label, and rgb
             - nv: RGB tuple for null values (or None)
             - default: RGB tuple for default color (or None)
    :rtype: dict
    """
    raw = gs.read_command("r.colors.out", map=mapname)
    lines = raw.strip().splitlines()

    items = []
    nv = None
    default = None

    # Detect categorical vs continuous
    numeric_values = []

    for line in lines:
        parts = line.split()
        if parts[0] in {"nv", "default"}:
            continue
        numeric_values.append(float(parts[0]))

    is_categorical = all(v.is_integer() for v in numeric_values)

    for line in lines:
        parts = line.split()
        if parts[0] == "nv":
            nv = tuple(map(int, parts[1].split(":")))
        elif parts[0] == "default":
            default = tuple(map(int, parts[1].split(":")))
        else:
            value = float(parts[0])
            rgb = tuple(map(int, parts[1].split(":")))

            label = f"Class {int(value)}" if is_categorical else f"{value:g}"

            items.append({"value": value, "label": label, "rgb": rgb})

    return {
        "type": "categorical" if is_categorical else "continuous",
        "items": items,
        "nv": nv,
        "default": default,
    }


def generate_legend_html(parsed, title="Legend", max_items=12):
    """Generate HTML for a raster legend.

    :param dict parsed: Parsed color information from parse_colors()
    :param str title: Title to display in the legend
    :param int max_items: Maximum number of items to display for continuous rasters
    :return: HTML string for the legend
    :rtype: str
    """
    items = parsed["items"]
    legend_type = parsed["type"]

    # Limit number of items for continuous rasters
    if legend_type == "continuous" and len(items) > max_items:
        step = max(1, len(items) // max_items)
        items = items[::step]

    html = [
        (
            '<div style="'
            "background: #fff;"
            "padding: 8px 10px;"
            "border: 1px solid #ccc;"
            "border-radius: 6px;"
            "font-size: 0.85em;"
            "font-family: sans-serif;"
            "max-height: 200px;"
            "overflow-y: auto;"
            "min-width: 120px;"
            "box-shadow: 0 1px 4px rgba(0,0,0,0.3);"
            '">'
        )
    ]

    html.append(f"<strong>{title}</strong><br>")

    for item in items:
        r, g, b = item["rgb"]
        html.append(
            f'<div style="display: flex; align-items: center; gap: 6px; '
            f'margin: 2px 0; white-space: nowrap;">'
            f'<span style="width: 14px; height: 14px; '
            f"background: rgb({r},{g},{b}); "
            f"display: inline-block; "
            f'border: 1px solid #000;"></span>'
            f"<span>{item['label']}</span>"
            f"</div>"
        )

    html.append("</div>")
    return "\n".join(html)
