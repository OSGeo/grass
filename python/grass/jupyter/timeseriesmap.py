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

import grass.script as gs
from grass.tools import Tools

from .region import RegionManagerForTimeSeries
from .baseseriesmap import BaseSeriesMap


def fill_none_values(names):
    """Replace `None` values in array with previous item"""
    for i, name in enumerate(names):
        if name is None:
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
    tools = Tools()
    if element_type == "strds":
        result = tools.t_rast_list(method="gran", input=timeseries, format="json")
    elif element_type == "stvds":
        result = tools.t_vect_list(method="gran", input=timeseries, format="json")
    else:
        raise NameError(
            _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries)
        )
    # Get layer names and start time from json
    names = [item["name"] for item in result["data"]]
    dates = [item["start_time"] for item in result["data"]]

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

    :Basic usage:
      .. code-block:: pycon

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
        self._baseseries = None

        # Handle Regions
        self._region_manager = RegionManagerForTimeSeries(
            use_region, saved_region, self._env
        )

    def add_raster_series(self, baseseries, fill_gaps=False, **kwargs):
        """
        :param str baseseries: name of space-time dataset
        :param bool fill_gaps: fill empty time steps with data from previous step
        """
        if self._baseseries_added and self._baseseries != baseseries:
            msg = "Cannot add more than one space time dataset"
            raise AttributeError(msg)
        self._element_type = "strds"
        check_timeseries_exists(baseseries, self._element_type)
        self._baseseries = baseseries
        self._fill_gaps = fill_gaps
        self._baseseries_added = True

        # create list of layers to render and date/times
        self._layers, self._labels = collect_layers(
            self._baseseries, self._element_type, self._fill_gaps
        )
        for raster in self._layers:
            kwargs["map"] = raster
            if raster is None:
                self._calls.append([(None, None)])
            else:
                self._calls.append([("d.rast", kwargs.copy())])
        self._date_layer_dict = {
            self._labels[i]: self._layers[i] for i in range(len(self._labels))
        }
        # Update Region
        self._region_manager.set_region_from_timeseries(self._baseseries)
        self._indices = self._labels

    def add_vector_series(self, baseseries, fill_gaps=False, **kwargs):
        """
        :param str baseseries: name of space-time dataset
        :param bool fill_gaps: fill empty time steps with data from previous step
        """
        if self._baseseries_added and self._baseseries != baseseries:
            msg = "Cannot add more than one space time dataset"
            raise AttributeError(msg)
        self._element_type = "stvds"
        check_timeseries_exists(baseseries, self._element_type)
        self._baseseries = baseseries
        self._fill_gaps = fill_gaps
        self._baseseries_added = True

        # create list of layers to render and date/times
        self._layers, self._labels = collect_layers(
            self._baseseries, self._element_type, self._fill_gaps
        )
        for vector in self._layers:
            kwargs["map"] = vector
            if vector is None:
                self._calls.append([(None, None)])
            else:
                self._calls.append([("d.vect", kwargs.copy())])
        self._date_layer_dict = {
            self._labels[i]: self._layers[i] for i in range(len(self._labels))
        }
        # Update Region
        self._region_manager.set_region_from_timeseries(
            self._baseseries, element_type="stvds"
        )
        self._indices = self._labels

    def d_legend(self, **kwargs):
        """Display legend.

        Wraps d.legend and uses same keyword arguments.
        """
        if "raster" in kwargs and not self._baseseries_added:
            self._base_layer_calls.append(("d.legend", kwargs))
        if "raster" in kwargs and self._baseseries_added:
            for i in range(len(self._layers)):
                self._calls[i].append(("d.legend", kwargs))
        else:
            info = gs.parse_command(
                "t.info", input=self._baseseries, flags="g", env=self._env
            )
            for i in range(len(self._layers)):
                self._calls[i].append(
                    (
                        "d.legend",
                        dict(
                            raster=self._layers[0],
                            range=f"{info['min_min']},{info['max_max']}",
                            **kwargs,
                        ),
                    )
                )
