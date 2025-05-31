# MODULE:    grass.jupyter.timeseriesmap
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#            Riya Saxena <29riyasaxena AT gmail>
#
# PURPOSE:   This module contains functions for visualizing raster and vector
#            space-time datasets in Jupyter Notebooks
#
# COPYRIGHT: (C) 2022-2024 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.
"""Create and display visualizations for space-time datasets."""

import os
import shutil

import grass.script as gs

from .map import Map
from .region import RegionManagerForTimeSeries
from .baseseriesmap import BaseSeriesMap


def fill_none_values(names):
    """Replace `None` values in array with previous item"""
    for i, name in enumerate(names):
        if name == "None":
            names[i] = names[i - 1]
    return names


def collect_layers(timeseries, element_type, fill_gaps):
    """Create lists of layer names and start_times for a
    space-time raster or vector dataset.

    For datasets with variable time steps, makes step regular with
    "gran" method for t.rast.list or t.vect.list then fills in
    missing layers with previous time step layer.

    :param str timeseries: name of space-time dataset
    :param str element_type: element type, "stvds" or "strds"
    :param bool fill_gaps: fill empty time steps with data from previous step
    """
    # NEW WAY: Comment in after json output for t.rast.list and t.vect.list is merged
    # import json
    # if element_type == "strds":
    #     result = json.loads(
    #         gs.read_command(
    #             "t.rast.list", method="gran", input=timeseries, format="json"
    #         )
    #     )
    # elif element_type == "stvds":
    #     result = json.loads(
    #         gs.read_command(
    #             "t.vect.list", method="gran", input=timeseries, format="json"
    #         )
    #     )
    # else:
    #     raise NameError(
    #         _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries)
    #     )
    #
    # # Get layer names and start time from json
    # names = [item["name"] for item in result["data"]]
    # dates = [item["start_time"] for item in result["data"]]

    if element_type == "strds":
        rows = gs.read_command(
            "t.rast.list", method="gran", input=timeseries
        ).splitlines()
    elif element_type == "stvds":
        rows = gs.read_command(
            "t.vect.list", method="gran", input=timeseries
        ).splitlines()
    else:
        raise NameError(
            _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries)
        )

    # Parse string
    # Create list of list
    new_rows = [row.split("|") for row in rows]
    # Transpose into columns where the first value is the name of the column
    new_array = [list(row) for row in zip(*new_rows)]

    # Collect layer name and start time
    for column in new_array:
        if column[0] == "name":
            names = column[1:]
        if column[0] == "start_time":
            dates = column[1:]

    # For datasets with variable time steps, fill in gaps with
    # previous time step value, if fill_gaps==True.
    if fill_gaps:
        names = fill_none_values(names)

    return names, dates


def check_timeseries_exists(timeseries, element_type):
    """Check that timeseries is time space dataset"""
    test = gs.read_command("t.list", type=element_type, where=f"name='{timeseries}'")
    if not test:
        raise NameError(
            _("Could not find space time dataset named {} of type {}").format(
                timeseries, element_type
            )
        )


class TimeSeriesMap(BaseSeriesMap):
    """Creates visualizations of time-space raster and vector datasets in Jupyter
    Notebooks.

    Basic usage::

    >>> img = TimeSeriesMap("series_name")
    >>> img.d_legend()  # Add legend
    >>> img.show()  # Create TimeSlider
    >>> img.save("image.gif")

    This class of grass.jupyter is experimental and under development. The API can
    change at anytime.
    """

    # pylint: disable=too-many-instance-attributes
    # Need more attributes to build timeseriesmap visuals
    # pylint: disable=duplicate-code

    def __init__(
        self,
        width=None,
        height=None,
        env=None,
        use_region=False,
        saved_region=None,
    ):
        """Creates an instance of the TimeSeriesMap visualizations class.

        :param int width: width of map in pixels
        :param int height: height of map in pixels
        :param str env: environment
        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        """
        super().__init__(width, height, env)

        self._element_type = None
        self._fill_gaps = None
        self._legend = None
        self._layers = None
        self._date_layer_dict = {}
        self._slider_description = _("Date/Time")

        # Handle Regions
        self._region_manager = RegionManagerForTimeSeries(
            use_region, saved_region, self._env
        )

    def add_raster_series(self, baseseries, fill_gaps=False):
        """
        :param str baseseries: name of space-time dataset
        :param bool fill_gaps: fill empty time steps with data from previous step
        """
        if self._baseseries_added and self.baseseries != baseseries:
            msg = "Cannot add more than one space time dataset"
            raise AttributeError(msg)
        self._element_type = "strds"
        check_timeseries_exists(baseseries, self._element_type)
        self.baseseries = baseseries
        self._fill_gaps = fill_gaps
        self._baseseries_added = True
        # create list of layers to render and date/times
        self._layers, self._labels = collect_layers(
            self.baseseries, self._element_type, self._fill_gaps
        )
        self._date_layer_dict = {
            self._labels[i]: self._layers[i] for i in range(len(self._labels))
        }
        # Update Region
        self._region_manager.set_region_from_timeseries(self.baseseries)
        self._indices = self._labels

    def add_vector_series(self, baseseries, fill_gaps=False):
        """
        :param str baseseries: name of space-time dataset
        :param bool fill_gaps: fill empty time steps with data from previous step
        """
        if self._baseseries_added and self.baseseries != baseseries:
            msg = "Cannot add more than one space time dataset"
            raise AttributeError(msg)
        self._element_type = "stvds"
        check_timeseries_exists(baseseries, self._element_type)
        self.baseseries = baseseries
        self._fill_gaps = fill_gaps
        self._baseseries_added = True
        # create list of layers to render and date/times
        self._layers, self._labels = collect_layers(
            self.baseseries, self._element_type, self._fill_gaps
        )
        self._date_layer_dict = {
            self._labels[i]: self._layers[i] for i in range(len(self._labels))
        }
        # Update Region
        self._region_manager.set_region_from_timeseries(self.baseseries)
        self._indices = self._labels

    def d_legend(self, **kwargs):
        """Display legend.

        Wraps d.legend and uses same keyword arguments.
        """
        if "raster" in kwargs and not self._baseseries_added:
            self._base_layer_calls.append(("d.legend", kwargs))
        if "raster" in kwargs and self._baseseries_added:
            self._base_calls.append(("d.legend", kwargs))
        else:
            self._legend = kwargs
            # If d_legend has been called, we need to re-render layers
            self._layers_rendered = False

    def _render_legend(self, img):
        """Add legend to Map instance"""
        info = gs.parse_command(
            "t.info", input=self.baseseries, flags="g", env=self._env
        )
        min_min = info["min_min"]
        max_max = info["max_max"]
        img.d_legend(
            raster=self._layers[0],
            range=f"{min_min}, {max_max}",
            **self._legend,
        )

    def _render_overlays(self, img):
        """Add collected overlays to Map instance"""
        for grass_module, kwargs in self._base_calls:
            img.run(grass_module, **kwargs)

    def _render_blank_layer(self, filename):
        """Write blank image for gaps in time series.

        Adds overlays and legend to base map.
        """
        img = Map(
            width=self._width,
            height=self._height,
            filename=filename,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        # Add overlays
        self._render_overlays(img)
        # Add legend if needed
        if self._legend:
            self._render_legend(img)

    def _render_layer(self, layer, filename):
        """Render layer to file with overlays and legend"""
        img = Map(
            width=self._width,
            height=self._height,
            filename=filename,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        if self._element_type == "strds":
            img.d_rast(map=layer)
        elif self._element_type == "stvds":
            img.d_vect(map=layer)
        # Add overlays
        self._render_overlays(img)
        # Add legend if needed
        if self._legend:
            self._render_legend(img)

    def _render_worker(self, date, layer, filename):
        """Function to render a single layer."""
        shutil.copyfile(self.base_file, filename)
        if layer == "None":
            self._render_blank_layer(filename)
        else:
            self._render_layer(layer, filename)
        return date, filename

    def render(self):
        """Renders image for each time-step in space-time dataset."""
        if not self._baseseries_added:
            msg = (
                "Cannot render space time dataset since none has been added."
                " Use TimeSeriesMap.add_raster_series() or "
                "TimeSeriesMap.add_vector_series() to add dataset"
            )
            raise RuntimeError(msg)

        # Create name for empty layers
        random_name_none = gs.append_random("none", 8) + ".png"

        # Prepare tasks with tuples
        tasks = []
        for date, layer in self._date_layer_dict.items():
            if layer == "None":
                filename = os.path.join(self._tmpdir.name, random_name_none)
            else:
                filename = os.path.join(self._tmpdir.name, f"{layer}.png")
            tasks.append((date, layer, filename))
        self._render(tasks)
