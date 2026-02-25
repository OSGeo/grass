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

    Detects raster datatype using r.info to determine whether
    the legend should be categorical (CELL) or continuous
    (FCELL/DCELL), similar to how d.legend behaves in GRASS.

    :param str mapname: Name of the raster map
    :return: Dictionary containing color information with keys:
             - type: "categorical" or "continuous"
             - items: list of dicts with value, label, and rgb
             - nv: RGB tuple for null values (or None)
             - default: RGB tuple for default color (or None)
    :rtype: dict
    """
    # Detect raster datatype using r.info
    info = gs.raster_info(mapname)
    datatype = info.get("datatype", "CELL")
    is_categorical = datatype == "CELL"

    raw = gs.read_command("r.colors.out", map=mapname)
    lines = raw.strip().splitlines()

    items = []
    nv = None
    default = None

    for line in lines:
        parts = line.split()
        if parts[0] == "nv":
            nv = tuple(map(int, parts[1].split(":")))
        elif parts[0] == "default":
            default = tuple(map(int, parts[1].split(":")))
        else:
            value = float(parts[0])
            rgb = tuple(map(int, parts[1].split(":")))

            if is_categorical:
                label = f"Class {int(value)}"
            else:
                label = f"{value:g}"

            items.append({"value": value, "label": label, "rgb": rgb})

    return {
        "type": "categorical" if is_categorical else "continuous",
        "items": items,
        "nv": nv,
        "default": default,
    }


def _generate_categorical_html(items, title):
    """Generate categorical legend HTML with color boxes.

    :param list items: List of color items
    :param str title: Legend title
    :return: HTML string
    :rtype: str
    """
    html = [
        (
            '<div class="maplegend leaflet-control" style="'
            "pointer-events: auto;"
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


def _generate_continuous_html(items, title):
    """Generate continuous gradient legend HTML with a color bar.

    Creates a vertical gradient bar similar to d.legend in GRASS,
    with min and max labels and intermediate tick marks.

    :param list items: List of color items (breakpoints)
    :param str title: Legend title
    :return: HTML string
    :rtype: str
    """
    if not items:
        return ""

    # Build CSS gradient stops
    min_val = items[0]["value"]
    max_val = items[-1]["value"]
    val_range = max_val - min_val if max_val != min_val else 1

    gradient_stops = []
    # Reverse items so stops go from top (0%) to bottom (100%)
    for item in reversed(items):
        r, g, b = item["rgb"]
        # Calculate position as percentage (0% is top/max, 100% is bottom/min)
        pct = 100 - ((item["value"] - min_val) / val_range * 100)
        gradient_stops.append(f"rgb({r},{g},{b}) {pct:.1f}%")

    gradient_css = ", ".join(gradient_stops)

    # Select tick labels (show ~5 evenly spaced labels)
    num_ticks = min(5, len(items))
    if num_ticks > 1:
        step = max(1, (len(items) - 1) // (num_ticks - 1))
        tick_items = items[::step]
        # Always include the last item
        if tick_items[-1] != items[-1]:
            tick_items.append(items[-1])
    else:
        tick_items = items

    # Build tick marks HTML
    ticks_html = []
    for item in tick_items:
        pct = 100 - ((item["value"] - min_val) / val_range * 100)
        ticks_html.append(
            f'<div style="position: absolute; right: 0; '
            f"top: {pct:.1f}%; transform: translateY(-50%); "
            f'font-size: 0.8em; white-space: nowrap;">'
            f"&#8212; {item['label']}"
            f"</div>"
        )

    html = [
        (
            '<div class="maplegend leaflet-control" style="'
            "pointer-events: auto;"
            "background: #fff;"
            "padding: 8px 10px;"
            "border: 1px solid #ccc;"
            "border-radius: 6px;"
            "font-size: 0.85em;"
            "font-family: sans-serif;"
            "min-width: 60px;"
            "box-shadow: 0 1px 4px rgba(0,0,0,0.3);"
            '">'
        ),
        f"<strong>{title}</strong>",
        '<div style="display: flex; gap: 4px; margin-top: 6px;">',
        # Gradient bar
        (
            '<div style="width: 20px; height: 150px; '
            f"background: linear-gradient(to bottom, {gradient_css}); "
            'border: 1px solid #000;"></div>'
        ),
        # Tick labels
        '<div style="position: relative; width: 60px; height: 150px;">',
        "\n".join(ticks_html),
        "</div>",
        "</div>",
        "</div>",
    ]

    return "\n".join(html)


def generate_legend_html(parsed, title="Legend", max_items=12):
    """Generate HTML for a raster legend.

    For categorical (CELL) rasters, generates a legend with
    color boxes. For continuous (FCELL/DCELL) rasters, generates
    a gradient color bar similar to d.legend in GRASS.

    :param dict parsed: Parsed color information from parse_colors()
    :param str title: Title to display in the legend
    :param int max_items: Maximum number of items for categorical legends
    :return: HTML string for the legend
    :rtype: str
    """
    items = parsed["items"]
    legend_type = parsed["type"]

    if legend_type == "continuous":
        return _generate_continuous_html(items, title)

    # Categorical: limit items if needed
    if len(items) > max_items:
        step = max(1, len(items) // max_items)
        items = items[::step]

    return _generate_categorical_html(items, title)
