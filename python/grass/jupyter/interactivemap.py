#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for interactive visualizations
#            in Jupyter Notebooks.
#
# COPYRIGHT: (C) 2021-2022 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Interactive visualizations map with folium"""

from .reprojection_renderer import ReprojectionRenderer


class Raster:
    """Overlays rasters on folium maps.

    Basic Usage:
    >>> m = folium.Map()
    >>> gj.Raster("elevation", opacity=0.5).add_to(m)
    >>> m
    """

    def __init__(
        self,
        name,
        title=None,
        use_region=False,
        saved_region=None,
        renderer=None,
        **kwargs,
    ):
        """Reproject GRASS raster, export to PNG, and compute bounding box.

        param str name: raster name
        param str title: title of raster to display in layer control legend
        param bool use_region: use computational region of current mapset
        param str saved_region: name of saved computation region
        param renderer: instance of ReprojectionRenderer
        **kwargs: keyword arguments passed to folium.raster_layers.ImageOverlay()
        """
        import folium  # pylint: disable=import-outside-toplevel

        self._folium = folium

        self._name = name
        self._overlay_kwargs = kwargs
        self._title = title
        if not self._title:
            self._title = self._name

        if not renderer:
            self._renderer = ReprojectionRenderer(
                use_region=use_region, saved_region=saved_region
            )
        else:
            self._renderer = renderer
        # Render overlay
        # By doing this here instead of in add_to, we avoid rendering
        # twice if added to multiple maps. This mimics the behavior
        # folium.raster_layers.ImageOverlay()
        self._filename, self._bounds = self._renderer.render_raster(name)

    def add_to(self, folium_map):
        """Add raster to folium map with folium.raster_layers.ImageOverlay()

        A folium map is an instance of folium.Map.
        """
        # Overlay image on folium map
        img = self._folium.raster_layers.ImageOverlay(
            image=self._filename,
            bounds=self._bounds,
            name=self._title,
            **self._overlay_kwargs,
        )

        # Add image to map
        img.add_to(folium_map)


class Vector:
    """Adds vectors to a folium map.

    Basic Usage:
    >>> m = folium.Map()
    >>> gj.Vector("roadsmajor").add_to(m)
    >>> m
    """

    def __init__(
        self,
        name,
        title=None,
        use_region=False,
        saved_region=None,
        renderer=None,
        **kwargs,
    ):
        """Reproject GRASS vector and export to folium-ready PNG. Also computes bounding
        box for PNG overlay in folium map.

        param str name: vector name
        param str title: title of vector to display in layer control legend
        param bool use_region: use computational region of current mapset
        param str saved_region: name of saved computation region
        renderer: instance of ReprojectionRenderer
        **kwargs: keyword arguments passed to folium.GeoJson()
        """
        import folium  # pylint: disable=import-outside-toplevel

        self._folium = folium

        self._name = name
        self._title = title
        if not self._title:
            self._title = self._name
        self._geojson_kwargs = kwargs

        if not renderer:
            self._renderer = ReprojectionRenderer(
                use_region=use_region, saved_region=saved_region
            )
        else:
            self._renderer = renderer
        self._filename = self._renderer.render_vector(name)

    def add_to(self, folium_map):
        """Add vector to folium map with folium.GeoJson()"""
        self._folium.GeoJson(
            str(self._filename), name=self._title, **self._geojson_kwargs
        ).add_to(folium_map)


class InteractiveMap:
    """This class creates interactive GRASS maps with folium.

    Basic Usage:

    >>> m = InteractiveMap()
    >>> m.add_vector("streams")
    >>> m.add_raster("elevation")
    >>> m.add_layer_control()
    >>> m.show()
    """

    def __init__(
        self,
        width=400,
        height=400,
        tiles="CartoDB positron",
        API_key=None,  # pylint: disable=invalid-name
        use_region=False,
        saved_region=None,
    ):
        """Creates a blank folium map centered on g.region.

        tiles parameter is passed directly to folium.Map() which supports several
        built-in tilesets (including "OpenStreetMap", "Stamen Toner", "Stamen Terrain",
        "Stamen Watercolor", "Mapbox Bright", "Mapbox Control Room", "CartoDB positron",
        "CartoDB dark_matter") as well as custom tileset URL (i.e.
        "http://{s}.yourtiles.com/{z}/{x}/{y}.png"). For more information, visit
        folium documentation:
        https://python-visualization.github.io/folium/modules.html

        Raster and vector data are always reprojected to Pseudo-Mercator.
        With use_region=True or saved_region=myregion, the region extent
        is reprojected and the number of rows and columns of that region
        is kept the same. This region is then used for reprojection.
        By default, use_region is False, which results in the
        reprojection of the entire raster in its native resolution.
        The reprojected resolution is estimated with r.proj.
        Vector data are always reprojected without any clipping,
        i.e., region options don't do anything.

        :param int height: height in pixels of figure (default 400)
        :param int width: width in pixels of figure (default 400)
        :param str tiles: map tileset to use
        :param str API_key: API key for Mapbox or Cloudmade tiles
        :param bool use_region: use computational region of current mapset
        :param str saved_region: name of saved computation region
        """
        import folium  # pylint: disable=import-outside-toplevel

        self._folium = folium

        # Store height and width
        self.width = width
        self.height = height

        # Create Folium Map
        self.map = self._folium.Map(
            width=self.width,
            height=self.height,
            tiles=tiles,
            API_key=API_key,  # pylint: disable=invalid-name
        )
        # Set LayerControl default
        self.layer_control = False
        self.layer_control_object = None

        self._renderer = ReprojectionRenderer(
            use_region=use_region, saved_region=saved_region
        )

    def add_vector(self, name, title=None, **kwargs):
        """Imports vector into temporary WGS84 location, re-formats to a GeoJSON and
        adds to folium map.

        :param str name: name of vector to be added to map;
                         positional-only parameter
        :param str title: vector name for layer control
        :**kwargs: keyword arguments passed to folium.GeoJson()
        """
        Vector(name, title=title, renderer=self._renderer, **kwargs).add_to(self.map)

    def add_raster(self, name, title=None, **kwargs):
        """Imports raster into temporary WGS84 location,
        exports as png and overlays on folium map.

        Color table for the raster can be modified with `r.colors` before calling
        this function.

        .. note:: This will only work if the raster is located in the current mapset.
        To change the color table of a raster located outside the current mapset,
        switch to that mapset with `g.mapset`, modify the color table with `r.color`
        then switch back to the initial mapset and run this function.

        :param str name: name of raster to add to display; positional-only parameter
        :param str title: raster name for layer control
        :**kwargs: keyword arguments passed to folium.raster_layers.ImageOverlay()
        """
        Raster(name, title=title, renderer=self._renderer, **kwargs).add_to(self.map)

    def add_layer_control(self, **kwargs):
        """Add layer control to display"

        Accepts keyword arguments to be passed to folium.LayerControl()"""

        self.layer_control = True
        self.layer_control_object = self._folium.LayerControl(**kwargs)

    def show(self):
        """This function returns a folium figure object with a GRASS raster
        overlaid on a basemap.

        If map has layer control enabled, additional layers cannot be
        added after calling show()."""

        if self.layer_control:
            self.map.add_child(self.layer_control_object)
        # Create Figure
        fig = self._folium.Figure(width=self.width, height=self.height)
        # Add map to figure
        fig.add_child(self.map)
        self.map.fit_bounds(self._renderer.get_bbox())

        return fig

    def save(self, filename):
        """Save map as an html map.

        :param str filename: name of html file
        """
        self.map.save(filename)
