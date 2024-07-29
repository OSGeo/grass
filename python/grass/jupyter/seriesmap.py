# MODULE:    grass.jupyter.seriesmap
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for visualizing series of rasters in
#            Jupyter Notebooks
#
# COPYRIGHT: (C) 2022 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.
"""Create and display visualizations for a series of rasters."""

import os
import shutil

from grass.grassdb.data import map_exists

from .map import Map
from .region import RegionManagerForSeries
from .utils import save_gif
from .baseseriesmap import BaseSeriesMap


class SeriesMap(BaseSeriesMap):
    """Creates visualizations from a series of rasters or vectors in Jupyter
    Notebooks.

    Basic usage::

    >>> series = gj.SeriesMap(height = 500)
    >>> series.add_rasters(["elevation_shade", "geology", "soils"])
    >>> series.add_vectors(["streams", "streets", "viewpoints"])
    >>> series.d_barscale()
    >>> series.show()  # Create Slider
    >>> series.save("image.gif")

    This class of grass.jupyter is experimental and under development. The API can
    change at anytime.
    """

    # pylint: disable=too-many-instance-attributes
    # pylint: disable=duplicate-code

    def __init__(
        self,
        width=None,
        height=None,
        env=None,
        use_region=False,
        saved_region=None,
    ):
        """Creates an instance of the SeriesMap visualizations class.

        :param int width: width of map in pixels
        :param int height: height of map in pixels
        :param str env: environment
        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        """
        super().__init__(width, height, env)

        # Handle Regions
        self._region_manager = RegionManagerForSeries(
            use_region=use_region,
            saved_region=saved_region,
            width=width,
            height=height,
            env=self._env,
        )

    def add_rasters(self, rasters, **kwargs):
        """
        :param list rasters: list of raster layers to add to SeriesMap
        """
        for raster in rasters:
            if not map_exists(name=raster, element="raster"):
                raise NameError(_("Could not find a raster named {}").format(raster))
        # Update region to rasters if not use_region or saved_region
        self._region_manager.set_region_from_rasters(rasters)
        if self._baseseries_added:
            assert self.baseseries == len(rasters), _(
                "Number of vectors in series must match number of vectors"
            )
            for i in range(self.baseseries):
                kwargs["map"] = rasters[i]
                self._base_calls[i].append(("d.rast", kwargs.copy()))
        else:
            self.baseseries = len(rasters)
            for raster in rasters:
                kwargs["map"] = raster
                self._base_calls.append([("d.rast", kwargs.copy())])
            self._baseseries_added = True
        if not self._labels:
            self._labels = rasters
        self._layers_rendered = False
        self._indices = list(range(len(self._labels)))

    def add_vectors(self, vectors, **kwargs):
        """
        :param list vectors: list of vector layers to add to SeriesMap
        """
        for vector in vectors:
            if not map_exists(name=vector, element="vector"):
                raise NameError(_("Could not find a vector named {}").format(vector))
        # Update region extent to vectors if not use_region or saved_region
        self._region_manager.set_region_from_vectors(vectors)
        if self._baseseries_added:
            assert self.baseseries == len(vectors), _(
                "Number of rasters in series must match number of vectors"
            )
            for i in range(self.baseseries):
                kwargs["map"] = vectors[i]
                self._base_calls[i].append(("d.vect", kwargs.copy()))
        else:
            self.baseseries = len(vectors)
            for vector in vectors:
                kwargs["map"] = vector
                self._base_calls.append([("d.vect", kwargs.copy())])
            self._baseseries_added = True
        if not self._labels:
            self._labels = vectors
        self._layers_rendered = False
        self._indices = range(len(self._labels))

    def add_names(self, names):
        """Add list of names associated with layers.
        Default will be names of first series added."""
        assert self.baseseries == len(names), _(
            "Number of vectors in series must match number of vectors"
        )
        self._labels = names
        self._indices = list(range(len(self._labels)))

    def render(self):
        """Renders image for each raster in series.

        Save PNGs to temporary directory. Must be run before creating a visualization
        (i.e. show or save).
        """
        self._render()
        if not self._baseseries_added:
            raise RuntimeError(
                "Cannot render series since none has been added."
                "Use SeriesMap.add_rasters() or SeriesMap.add_vectors()"
            )

        # Render each layer
        for i in range(self.baseseries):
            # Create file
            filename = os.path.join(self._tmpdir.name, f"{i}.png")
            # Copying the base_file ensures that previous results are overwritten
            shutil.copyfile(self.base_file, filename)
            self._base_filename_dict[i] = filename
            # Render image
            img = Map(
                width=self._width,
                height=self._height,
                filename=filename,
                use_region=True,
                env=self._env,
                read_file=True,
            )
            for grass_module, kwargs in self._base_calls[i]:
                img.run(grass_module, **kwargs)

        self._layers_rendered = True

    def save(
        self,
        filename,
        duration=500,
        label=True,
        font=None,
        text_size=12,
        text_color="gray",
    ):
        """
        Creates a GIF animation of rendered layers.

        Text color must be in a format accepted by PIL ImageColor module. For supported
        formats, visit:
        https://pillow.readthedocs.io/en/stable/reference/ImageColor.html#color-names

        param str filename: name of output GIF file
        param int duration: time to display each frame; milliseconds
        param bool label: include label on each frame
        param str font: font file
        param int text_size: size of label text
        param str text_color: color to use for the text
        """

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        tmp_files = []
        for file in self._base_filename_dict.values():
            tmp_files.append(file)

        save_gif(
            tmp_files,
            filename,
            duration=duration,
            label=label,
            labels=self._labels,
            font=font,
            text_size=text_size,
            text_color=text_color,
        )

        # Display the GIF
        return filename
