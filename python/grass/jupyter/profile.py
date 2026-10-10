# MODULE:    grass.jupyter.profile
#
# AUTHOR(S): Corey White <ctwhite AT ncsu edu>
#
# PURPOSE:   This module contains functions for sampling and plotting
#            raster values along a line in Jupyter Notebooks
#
# SPDX-FileCopyrightText: 2026 Corey White
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Raster profiles along a line"""

import bisect
import io
import os

from grass.tools import Tools

from .map import Map
from .utils import get_region


class Profile:
    """Values of one or more rasters sampled along a line.

    The line is given as coordinate pairs in the coordinate reference system
    of the current project. Values are sampled with *r.profile* and the
    distances of the line vertices are measured with *m.measure*.

    :Basic usage:
      .. code-block:: pycon

        >>> profile = Profile("elevation", [(638000, 220000), (640000, 222000)])
        >>> profile.data["elevation"][0]
        {'easting': 638000.0, 'northing': 220000.0, 'distance': 0, 'value': 109.5}
        >>> profile.save("profile.png")
    """

    def __init__(
        self,
        rasters,
        coordinates,
        *,
        env=None,
        session=None,
        resolution=None,
        units=None,
    ):
        """Samples the rasters along the line.

        :param rasters: name of a raster or a list of raster names
        :param coordinates: list of (east, north) tuples in the project CRS
        :param env: runtime environment to use for execution (defaults to global)
        :param session: session with environment (used if it has an env attribute)
        :param resolution: sampling step along the line in map units
                           (defaults to the mean of the region resolutions)
        :param units: distance units (meters, kilometers, feet, miles),
                      defaults to the project units
        """
        if isinstance(rasters, str):
            rasters = [rasters]
        self.rasters = list(rasters)
        if not self.rasters:
            raise ValueError(_("At least one raster is required"))
        self.coordinates = [(float(east), float(north)) for east, north in coordinates]
        if len(self.coordinates) < 2:
            raise ValueError(_("At least two coordinate pairs are required"))

        if env:
            self._env = env.copy()
        elif session and hasattr(session, "env"):
            self._env = session.env.copy()
        else:
            self._env = os.environ.copy()

        tools = Tools(env=self._env)
        units_kwargs = {"units": units} if units else {}
        self.vertices, self.distance_units = self._measure_vertices(tools, units_kwargs)

        profile_kwargs = dict(units_kwargs)
        if resolution is not None:
            profile_kwargs["resolution"] = resolution
        flat_coordinates = [value for pair in self.coordinates for value in pair]
        self.data = {
            raster: tools.r_profile(
                input=raster,
                coordinates=flat_coordinates,
                flags="g",
                format="json",
                **profile_kwargs,
            ).json
            for raster in self.rasters
        }
        self._append_last_vertex(tools)

    def _measure_vertices(self, tools, units_kwargs):
        """Returns the vertices with cumulative distances and the distance units."""
        east, north = self.coordinates[0]
        vertices = [{"easting": east, "northing": north, "distance": 0}]
        for east, north in self.coordinates[1:]:
            previous = vertices[-1]
            result = tools.m_measure(
                coordinates=[previous["easting"], previous["northing"], east, north],
                format="json",
                **units_kwargs,
            ).json
            vertices.append(
                {
                    "easting": east,
                    "northing": north,
                    "distance": previous["distance"] + result["length"],
                }
            )
        return vertices, result["units"]["length"]

    def _append_last_vertex(self, tools):
        """Adds a sample at the last vertex, which r.profile does not output."""
        east, north = self.coordinates[-1]
        values = tools.r_what(
            map=self.rasters, coordinates=[east, north], format="json"
        ).json[0]
        distance = self.vertices[-1]["distance"]
        for raster in self.rasters:
            self.data[raster].append(
                {
                    "easting": east,
                    "northing": north,
                    "distance": distance,
                    "value": values[raster]["value"],
                }
            )

    def to_dataframe(self):
        """Returns the samples as a pandas DataFrame with a raster column.

        Requires pandas.
        """
        import pandas as pd  # pylint: disable=import-outside-toplevel

        return pd.DataFrame(
            [
                {"raster": raster, **row}
                for raster, rows in self.data.items()
                for row in rows
            ]
        )

    def plot(self, ax=None, markers=True):
        """Plots the profile of each raster as distance against value.

        :param ax: matplotlib axes to draw on, a new figure is created if None
        :param bool markers: draw numbered markers at the line vertices
        :return: the matplotlib axes
        """
        from matplotlib.figure import Figure  # pylint: disable=import-outside-toplevel

        if ax is None:
            ax = Figure().subplots()
        for raster, rows in self.data.items():
            distances = [row["distance"] for row in rows]
            # None becomes NaN, which matplotlib leaves as a gap in the line.
            values = [
                float("nan") if row["value"] is None else row["value"] for row in rows
            ]
            (line,) = ax.plot(distances, values, linewidth=2, label=raster)
            if markers:
                for number, vertex in enumerate(self.vertices, start=1):
                    value = self._interpolate(rows, vertex["distance"])
                    if value is not None:
                        self._draw_marker(
                            ax, vertex["distance"], value, number, line.get_color()
                        )
        if len(self.data) > 1:
            ax.legend()
        ax.set_xlabel(_("Distance [{}]").format(self.distance_units))
        ax.set_ylabel(_("Value"))
        return ax

    @staticmethod
    def _interpolate(rows, distance):
        """Returns the value at a distance by linear interpolation between samples.

        Returns None when a neighboring sample is null so that no marker
        is drawn in a gap of the line.
        """
        distances = [row["distance"] for row in rows]
        index = bisect.bisect_right(distances, distance) - 1
        if index < 0:
            return None
        before = rows[index]
        if index + 1 >= len(rows) or before["distance"] == distance:
            return before["value"]
        after = rows[index + 1]
        if before["value"] is None or after["value"] is None:
            return None
        fraction = (distance - before["distance"]) / (
            after["distance"] - before["distance"]
        )
        return before["value"] + fraction * (after["value"] - before["value"])

    @staticmethod
    def _draw_marker(ax, x, y, number, color):
        """Draws a numbered marker."""
        ax.plot(
            x,
            y,
            marker="o",
            markersize=8,
            markerfacecolor="white",
            markeredgecolor=color,
            linestyle="none",
        )
        ax.annotate(
            str(number),
            (x, y),
            textcoords="offset points",
            xytext=(0, 7),
            ha="center",
            fontsize=8,
        )

    def _plot_map(self, ax, raster):
        """Draws the raster with the line and numbered vertices."""
        import matplotlib.image as mpimg  # pylint: disable=import-outside-toplevel

        # The current region is what r.profile sampled, so render exactly that.
        region = get_region(env=self._env)
        rendered = Map(env=self._env, use_region=True)
        rendered.d_rast(map=raster)
        image = mpimg.imread(rendered.filename)
        ax.imshow(
            image,
            extent=[region["west"], region["east"], region["south"], region["north"]],
        )
        eastings, northings = zip(*self.coordinates, strict=False)
        ax.plot(eastings, northings, color="black", linewidth=2)
        for number, (east, north) in enumerate(self.coordinates, start=1):
            self._draw_marker(ax, east, north, number, "black")
        ax.set_xticks([])
        ax.set_yticks([])
        ax.set_title(raster)

    def figure(self, raster=None, figsize=(10, 4)):
        """Creates a figure with a map of the line and the profile chart.

        :param raster: raster shown in the map panel (defaults to the first one)
        :param figsize: figure size in inches as (width, height)
        :return: matplotlib Figure
        """
        from matplotlib.figure import Figure  # pylint: disable=import-outside-toplevel

        fig = Figure(figsize=figsize, layout="constrained")
        map_ax, chart_ax = fig.subplots(1, 2, width_ratios=[1, 1.6])
        self._plot_map(map_ax, raster or self.rasters[0])
        self.plot(ax=chart_ax)
        return fig

    def show(self, **kwargs):
        """Displays the figure in the notebook.

        :param kwargs: arguments passed to :meth:`figure`
        """
        from IPython.display import (  # pylint: disable=import-outside-toplevel
            Image,
            display,
        )

        buffer = io.BytesIO()
        self.figure(**kwargs).savefig(buffer, format="png")
        display(Image(buffer.getvalue()))

    def save(self, filename, **kwargs):
        """Saves the figure to a file, the format follows the extension.

        :param filename: name of the image file
        :param kwargs: arguments passed to :meth:`figure`
        """
        self.figure(**kwargs).savefig(filename)
