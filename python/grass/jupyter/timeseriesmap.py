# MODULE:    grass.jupyter.timeseriesmap
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#            Riya Saxena <29riyasaxena AT gmail>
#
# PURPOSE:   This module contains functions for visualizing raster and vector
#            space-time datasets in Jupyter Notebooks
#
# COPYRIGHT: (C) 2022-2026 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.
"""Create and display visualizations for space-time datasets."""

from datetime import datetime

import grass.script as gs
from grass.tools import Tools

from .region import RegionManagerForTimeSeries
from .baseseriesmap import BaseSeriesMap


def fill_none_values(names: list[str]) -> list[str]:
    """Replace `None` values in array with previous item."""
    for i, name in enumerate(names):
        if name is None:
            names[i] = names[i - 1]
    return names


def collect_layers(
    timeseries: str,
    element_type: str,
    fill_gaps: bool,
) -> tuple[list, list]:
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
            _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries),
        )
    # Get layer names and start time from json
    names = [item["name"] for item in result["data"]]
    dates = [item["start_time"] for item in result["data"]]

    # For datasets with variable time steps, fill in gaps with
    # previous time step value, if fill_gaps==True.
    if fill_gaps:
        names = fill_none_values(names)

    return names, dates


def check_timeseries_exists(timeseries: str, element_type: str) -> None:
    """Check that timeseries is time space dataset."""
    test = gs.read_command("t.list", type=element_type, where=f"name='{timeseries}'")
    if not test:
        raise NameError(
            _("Could not find space time dataset named {} of type {}").format(
                timeseries,
                element_type,
            ),
        )


class TimeSeriesMap(BaseSeriesMap):
    """Create visualizations of time-space raster and vector datasets in Jupyter
    Notebooks.

    :Basic usage:
      .. code-block:: pycon

        >>> img = TimeSeriesMap()
        >>> img.add_raster_series("series_name")  # Add STRDS
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
        width: int = 600,
        height: int = 400,
        env: dict | None = None,
        use_region: bool = False,
        saved_region: str | None = None,
    ) -> None:
        """Create an instance of the TimeSeriesMap visualizations class.

        :param int width: width of map in pixels
        :param int height: height of map in pixels
        :param str env: environment
        :param bool use_region: if True, use either current or
                          provided saved region,
                          else derive region from rendered layers
        :param str saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        """
        super().__init__(width, height, env)

        # TGIS requires a GRASS session thus lazy import here
        global tgis
        import grass.temporal as tgis  # noqa: PLC0415

        tgis.init()

        self._fill_gaps = None
        self._legend = None
        self._layers = None
        self._date_layer_dict = {}
        self._slider_description = _("Date/Time")
        self._baseseries = None

        # Handle Regions
        self._region_manager = RegionManagerForTimeSeries(
            use_region,
            saved_region,
            self._env,
        )

    def _try_open_stds(self, stds_type: str) -> None:
        try:
            self._stds = tgis.open_old_stds(self._baseseries, stds_type)
            self._stds.select()
        except SystemExit:
            raise RuntimeError(
                _("Unable to open Space time dataset <%s> of type <%s>")
                % self._baseseries,
                stds_type,
            ) from None

    def _check_number_of_timesteps(
        self,
        start_time: datetime,
        end_time: datetime,
    ) -> None:
        """Inform about the number of maps to be added to the series map."""
        time_steps = (
            tgis.increment_datetime_by_string(start_time, self._granularity)
            - start_time
        )
        time_steps_n = (end_time - start_time) / time_steps
        gs.info(_("Adding %i raster maps to series map.") % time_steps_n)

    def _collect_layers(self) -> None:
        """Collect the maps to be added to the series map."""
        if self._granularity and not tgis.check_granularity_string(
            self._granularity,
            temporal_type=self._stds.get_temporal_type(),
        ):
            raise ValueError(
                _("Invalid granularity <%s> for Space Time Dataset <%s>.")
                % self._granularity,
                self._stds.get_id(),
            )
        if not self._granularity:
            self._granularity = self._stds.get_granularity()

        if not self._where:
            self._check_number_of_timesteps(
                self._stds.temporal_extent.start_time,
                self._stds.temporal_extent.end_time,
            )
            map_list = self._stds.get_registered_maps_as_objects_by_granularity(
                self._granularity,
            )
        else:
            map_list = self._stds.get_registered_maps_as_objects(
                where=self._where,
                order="start_time",
            )
            self._check_number_of_timesteps(
                map_list[0].temporal_extent.start_time,
                map_list[-1].temporal_extent.end_time,
            )
            map_list = tgis.AbstractSpaceTimeDataset.resample_maplist_by_granularity(
                map_list,
                map_list[0].get_temporal_extent().start_time,
                map_list[-1].get_temporal_extent().start_time,
                self._granularity,
            )
        self._layers = []
        self._labels = []
        for map_layer in map_list:
            if isinstance(map_layer, list):
                map_layer = next(iter(map_layer))
            self._layers.append(map_layer.get_id())
            self._labels.append(str(map_layer.get_temporal_extent().start_time))

    def _add_map_series(
        self,
        baseseries: str,
        fill_gaps: bool,
        where: str | None = None,
        granularity: str | None = None,
        element_type: str | None = None,
        **kwargs,  # noqa: ANN003
    ) -> None:
        """Add a map series to the TimeSeriesMap object.

        :param str baseseries: name of space-time dataset (STDS)
        :param str where: temporal where clause to select maps from STDS
        :param str granularity: granularity string (e.g. "1 day"),
            overrides granularity from STDS
        :param bool fill_gaps: fill empty time steps with data from
            previous step
        """
        if self._baseseries_added and self._baseseries != baseseries:
            msg = "Cannot add more than one space time dataset"
            raise AttributeError(msg)
        self._baseseries = baseseries
        self._fill_gaps = fill_gaps
        self._try_open_stds(element_type)
        self._baseseries_added = True
        self._where = where
        self._granularity = granularity

        renderer = {
            "strds": "d.rast",
            "stvds": "d.vect",
        }.get(element_type)

        # Create list of layers to render and date/times
        self._collect_layers()
        for series_map in self._layers:
            kwargs["map"] = series_map
            if series_map is None:
                self._calls.append([(None, None)])
            else:
                self._calls.append([(renderer, kwargs.copy())])
        self._date_layer_dict = {
            self._labels[i]: self._layers[i] for i in range(len(self._labels))
        }
        # Update Region
        self._region_manager.set_region_from_timeseries(self._stds)
        self._indices = self._labels

    def add_raster_series(
        self,
        baseseries: str,
        where: str | None = None,
        granularity: str | None = None,
        *,
        fill_gaps: bool = False,
        **kwargs,  # noqa: ANN003
    ) -> None:
        """Add a raster map series to the TimeSeriesMap object.

        :param str baseseries: name of space-time raster dataset (STRDS)
        :param str where: temporal where clause to select raster maps from STRDS
        :param str granularity: granularity string (e.g. "1 day"),
            overrides granularity from STRDS
        :param bool fill_gaps: fill empty time steps with data from previous step
        """
        self._add_map_series(
            baseseries,
            fill_gaps,
            where,
            granularity,
            element_type="strds",
            **kwargs,
        )

    def add_vector_series(
        self,
        baseseries: str,
        where: str,
        granularity: str,
        *,
        fill_gaps: bool = False,
        **kwargs,  # noqa: ANN003
    ) -> None:
        """Add a vector map series to the TimeSeriesMap object.

        :param str baseseries: name of space-time vector dataset (STVDS)
        :param str where: temporal where clause to select vector maps from STVDS
        :param str granularity: granularity string (e.g. "1 day"),
            overrides granularity from STVDS
        :param bool fill_gaps: fill empty time steps with data from previous step
        """
        self._add_map_series(
            baseseries,
            fill_gaps,
            where,
            granularity,
            element_type="stvds",
            **kwargs,
        )

    def d_legend(self, **kwargs) -> None:  # noqa: ANN003
        """Display legend.

        Wraps d.legend and uses same keyword arguments.
        """
        if "raster" in kwargs and not self._baseseries_added:
            self._base_layer_calls.append(("d.legend", kwargs))
        if "raster" in kwargs and self._baseseries_added:
            for i in range(len(self._layers)):
                self._calls[i].append(("d.legend", kwargs))
        else:
            for i in range(len(self._layers)):
                self._calls[i].append(
                    (
                        "d.legend",
                        dict(
                            raster=self._layers[0],
                            range=f"{self._stds.metadata.min_min},{self._stds.metadata.max_max}",
                            **kwargs,
                        ),
                    ),
                )
