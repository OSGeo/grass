#!/usr/bin/env python3
#
# make_synth_dem.py - generate the synthetic DEM used to benchmark r.param.scale
#
# WHAT THIS MAKES
#   A synthetic terrain raster (DCELL) of ROWS x COLS cells, default 10000 x 10000
#   (100 million cells, about 800 MB), named "synth_dem_100m" -- the default input
#   of the companion benchmark script. The surface is a deterministic sum of
#   sinusoids at several frequencies, built with r.mapcalc from col()/row() only, so
#   it is bit-reproducible on every machine and GRASS version (no random seed, no FFT).
#
#   r.surf.fractal would give more realistic fractal terrain, but it depends on that
#   module being built and on FFT-friendly dimensions; this analytic surface needs
#   only r.mapcalc and works at any size. r.param.scale's run time depends on the
#   raster size, not the cell values, so this is a valid and stable benchmark input.
#
# OVERWRITE
#   This OVERWRITES any existing raster named OUTPUT (it passes --overwrite). Do not
#   run it in a mapset where that name holds data you want to keep.
#
# RUN
#   grass /path/to/grassdata/<location>/<mapset> --exec python3 make_synth_dem.py
#   (use a projected location; r.param.scale does not support lat/long)

import grass.script as gs

# ---- configuration -------------------------------------------------------
ROWS = 10000  # lower ROWS/COLS for a quicker, smaller test (e.g. 2000 x 2000)
COLS = 10000
OUTPUT = "synth_dem_100m"
# --------------------------------------------------------------------------

# Deterministic multi-frequency surface (a sum of sinusoids of col()/row(), with a
# +500 baseline so values read like positive elevations). DCELL because the constants
# and sin()/cos() are double precision.
EXPR = (
    "double("
    "500.0 + "
    "256.0 * sin(col() * 0.9) * cos(row() * 0.9) + "
    "128.0 * sin(col() * 2.1) * cos(row() * 1.7) + "
    " 64.0 * sin(col() * 4.3) * cos(row() * 3.7) + "
    " 32.0 * sin((col() + row()) * 6.1)"
    ")"
)


def main():
    # Region: ROWS x COLS at resolution 1 (extent 0..COLS by 0..ROWS).
    gs.run_command("g.region", n=ROWS, s=0, e=COLS, w=0, res=1, quiet=True)
    gs.mapcalc("%s = %s" % (OUTPUT, EXPR), overwrite=True, quiet=True)
    info = gs.raster_info(OUTPUT)
    print(
        "created %s: %d x %d cells (%s)"
        % (OUTPUT, int(info["rows"]), int(info["cols"]), info["datatype"])
    )


if __name__ == "__main__":
    main()
