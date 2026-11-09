#!/usr/bin/env python3
"""Regenerate the v.ppa manual figures.

Creates clustered, random (CSR), and dispersed point patterns in a
temporary XY project, renders the pattern maps with d.vect, estimates
the G, F, K, and L functions with v.ppa, and plots them with
matplotlib. Writes v_ppa_patterns.png and v_ppa_{g,f,k,l}.png into the
directory given as the first argument (default: the script directory).

Requires a GRASS installation on PATH, its Python package on
PYTHONPATH, and matplotlib:

    export PYTHONPATH=$(grass --config python_path):$PYTHONPATH
    python3 doc_figures.py

Before committing, downsize is not needed (figures are created at 600px
width), but compress the images following the style guide, e.g.:

    mogrify -strip -dither None -colors 128 -format png v_ppa_*.png
"""

import os
import sys
import tempfile
from pathlib import Path

import matplotlib.image as mpimg
import matplotlib.pyplot as plt

import grass.script as gs
from grass.tools import Tools

INK = "#021905"
MUTED = "#4e4d4c"
GRID = "#d8d8d8"

# GRASS brand derived series colors, validated for colorblind safety.
SERIES = {
    "clustered": ("#b04c4c", "Clustered"),
    "random": ("#088b36", "Random (CSR)"),
    "dispersed": ("#2b6cb8", "Dispersed"),
}

plt.rcParams.update(
    {
        "font.family": "Fira Sans",
        "text.color": INK,
        "axes.edgecolor": MUTED,
        "axes.labelcolor": INK,
        "xtick.color": MUTED,
        "ytick.color": MUTED,
        "axes.titlesize": 12,
        "axes.labelsize": 10,
        "xtick.labelsize": 9,
        "ytick.labelsize": 9,
        "legend.fontsize": 9,
        "figure.facecolor": "white",
        "axes.facecolor": "white",
        "savefig.dpi": 100,
    }
)


def create_patterns(tools):
    """Create the three point patterns, about 200 points each."""
    tools.v_random(output="pattern_random", npoints=200, seed=42)

    tools.v_random(output="parents", npoints=10, seed=7)
    children = []
    for i in range(20):
        child = f"child_{i}"
        tools.v_perturb(
            input="parents",
            output=child,
            distribution="normal",
            parameters=[0, 25],
            seed=100 + i,
        )
        children.append(child)
    tools.v_patch(input=children, output="pattern_clustered")

    tools.v_mkgrid(grid=[14, 14], type="point", map="grid_points")
    tools.v_perturb(
        input="grid_points",
        output="pattern_dispersed",
        distribution="uniform",
        parameters=18,
        seed=5,
    )


def render_maps(tools, session, directory):
    """Render each pattern with d.vect and return the image paths."""
    session.env.update(
        {
            "GRASS_RENDER_IMMEDIATE": "cairo",
            "GRASS_RENDER_WIDTH": "700",
            "GRASS_RENDER_HEIGHT": "700",
            "GRASS_RENDER_BACKGROUNDCOLOR": "FFFFFF",
            "GRASS_RENDER_TRANSPARENT": "FALSE",
        }
    )
    paths = {}
    for pattern, (color, _) in SERIES.items():
        rgb = ":".join(str(int(color[i : i + 2], 16)) for i in (1, 3, 5))
        paths[pattern] = directory / f"map_{pattern}.png"
        session.env["GRASS_RENDER_FILE"] = str(paths[pattern])
        tools.d_vect(
            map=f"pattern_{pattern}",
            icon="basic/circle",
            size=6,
            color=rgb,
            fill_color=rgb,
        )
    return paths


def style_axes(ax):
    ax.spines[["top", "right"]].set_visible(False)
    ax.grid(True, color=GRID, linewidth=0.6)
    ax.set_axisbelow(True)
    ax.margins(x=0)


def plot_patterns(map_paths, out_dir):
    fig, axes = plt.subplots(1, 3, figsize=(6, 2.25))
    for ax, (pattern, (_, label)) in zip(axes, SERIES.items(), strict=True):
        ax.imshow(mpimg.imread(map_paths[pattern]))
        ax.set_title(label, fontsize=10)
        ax.set_xticks([])
        ax.set_yticks([])
        for spine in ax.spines.values():
            spine.set_color(GRID)
            spine.set_linewidth(0.8)
    fig.tight_layout()
    fig.savefig(out_dir / "v_ppa_patterns.png")
    plt.close(fig)


def plot_function(results, method, ylabel, title, csr_label, out_dir):
    fig, ax = plt.subplots(figsize=(6, 4.0))
    csr_key = f"{method}_csr"
    reference = results["random"][method]
    ax.plot(
        [r["distance"] for r in reference],
        [r[csr_key] for r in reference],
        color=MUTED,
        linewidth=1.6,
        linestyle=(0, (4, 3)),
        label=csr_label,
    )
    for pattern, (color, label) in SERIES.items():
        rows = results[pattern][method]
        ax.plot(
            [r["distance"] for r in rows],
            [r[method] for r in rows],
            color=color,
            linewidth=2,
            label=label,
        )
    ax.set_xlabel("Distance")
    ax.set_ylabel(ylabel)
    ax.set_title(title, loc="left", fontweight="bold", pad=10)
    style_axes(ax)
    legend_loc = "lower right" if method in {"g", "f"} else "upper left"
    ax.legend(loc=legend_loc, frameon=False)
    fig.tight_layout()
    fig.savefig(out_dir / f"v_ppa_{method}.png")
    plt.close(fig)


def main():
    out_dir = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).parent)
    out_dir.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory() as tmp_dir:
        project = Path(tmp_dir) / "ppa_figures"
        gs.create_project(project)
        with (
            gs.setup.init(project, env=os.environ.copy()) as session,
            Tools(session=session) as tools,
        ):
            tools.g_region(s=0, n=1000, w=0, e=1000, res=1)
            create_patterns(tools)
            results = {
                pattern: {
                    method: tools.v_ppa(
                        input=f"pattern_{pattern}",
                        method=method,
                        format="json",
                        seed=1,
                    ).json["results"]
                    for method in ("g", "f", "k", "l")
                }
                for pattern in SERIES
            }
            map_paths = render_maps(tools, session, Path(tmp_dir))
            plot_patterns(map_paths, out_dir)

    plot_function(
        results,
        "g",
        "G(d)",
        "G function: nearest neighbor distances",
        "CSR expectation",
        out_dir,
    )
    plot_function(
        results,
        "f",
        "F(d)",
        "F function: empty space distances",
        "CSR expectation",
        out_dir,
    )
    plot_function(
        results,
        "k",
        "K(d)",
        "Ripley's K function",
        "CSR expectation: πd²",
        out_dir,
    )
    plot_function(
        results,
        "l",
        "L(d)",
        "L function",
        "CSR expectation: L(d) = d",
        out_dir,
    )
    for name in sorted(p.name for p in out_dir.glob("v_ppa_*.png")):
        print(name)


if __name__ == "__main__":
    main()
