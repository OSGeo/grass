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

import operator

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

    from grass.tools import Tools

    tools = Tools()

    color_data = tools.r_colors_out(map=mapname, format="json", color_format="triplet")

    items = []
    nv = None
    default = None

    if "nv" in color_data:
        nv = tuple(map(int, color_data["nv"].split(":")))
    if "default" in color_data:
        default = tuple(map(int, color_data["default"].split(":")))

    for entry in color_data.get("table", []):
        value = float(entry["value"])
        rgb = tuple(map(int, entry["color"].split(":")))

        # JSON format outputs actual breakpoint numerical values directly
        label = f"Class {int(value)}" if is_categorical else f"{value:g}"

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

    # Sort items by value to ensure strictly ascending order
    items = sorted(items, key=operator.itemgetter("value"))

    # Build CSS gradient stops
    min_val = items[0]["value"]
    max_val = items[-1]["value"]
    val_range = max_val - min_val if max_val != min_val else 1

    # Ipywidgets HTML sanitizer aggressively strips linear-gradient().
    # To ensure cross-frontend compatibility, we manually interpolate the
    # color scale into 50 stacked solid-color div blocks.
    def get_interpolated_color(val):
        if val <= items[0]["value"]:
            return items[0]["rgb"]
        if val >= items[-1]["value"]:
            return items[-1]["rgb"]
        for i in range(len(items) - 1):
            if items[i]["value"] <= val <= items[i + 1]["value"]:
                v1, v2 = items[i]["value"], items[i + 1]["value"]
                r1, g1, b1 = items[i]["rgb"]
                r2, g2, b2 = items[i + 1]["rgb"]
                ratio = (val - v1) / (v2 - v1) if v2 > v1 else 0
                return (
                    int(r1 + (r2 - r1) * ratio),
                    int(g1 + (g2 - g1) * ratio),
                    int(b1 + (b2 - b1) * ratio),
                )
        return items[-1]["rgb"]

    gradient_html_blocks = []
    steps = 50
    for i in range(steps):
        # Top of the bar is max_val, bottom is min_val
        val = max_val - (i / max(1, steps - 1)) * val_range
        r, g, b = get_interpolated_color(val)
        gradient_html_blocks.append(
            f'<div style="flex-grow: 1; background: rgb({r},{g},{b});"></div>'
        )
    gradient_blocks_html = "".join(gradient_html_blocks)

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
            "display: flex; flex-direction: column; "
            'border: 1px solid #000;">'
        ),
        gradient_blocks_html,
        "</div>",
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
