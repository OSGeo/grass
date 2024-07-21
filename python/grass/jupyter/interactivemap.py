#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#            Anna Petrasova <kratochanna AT gmail>
#
# PURPOSE:   This module contains functions for interactive visualizations
#            in Jupyter Notebooks.
#
# COPYRIGHT: (C) 2021-2024 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Interactive visualizations map with folium or ipyleaflet"""

import base64
import json
from pathlib import Path
from .reprojection_renderer import ReprojectionRenderer
from .utils import (
    get_region_bounds_latlon,
    reproject_region,
    update_region,
    get_location_proj_string,
)


def get_backend(interactive_map):
    """Identifies if interactive_map is of type folium.Map
    or ipyleaflet.Map. Returns "folium" or "ipyleaflet".
    """
    try:
        import folium  # pylint: disable=import-outside-toplevel
    except ImportError:
        return "ipyleaflet"
    isfolium = isinstance(interactive_map, folium.Map)
    if isfolium:
        return "folium"
    return "ipyleaflet"


class Layer:  # pylint: disable=too-few-public-methods
    """Base class for overlaing raster or vector layer
    on a folium or ipyleaflet map.
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

        param str name: layer name
        param str title: title of layer to display in layer control legend
        param bool use_region: use computational region of current mapset
        param str saved_region: name of saved computation region
        param renderer: instance of ReprojectionRenderer
        **kwargs: keyword arguments passed to folium/ipyleaflet layer instance
        """
        self._name = name
        self._layer_kwargs = kwargs
        self._title = title
        if not self._title:
            self._title = self._name

        if not renderer:
            self._renderer = ReprojectionRenderer(
                use_region=use_region, saved_region=saved_region
            )
        else:
            self._renderer = renderer


class Raster(Layer):
    """Overlays rasters on a folium or ipyleaflet map.

    Basic Usage:
    >>> m = folium.Map()
    >>> gj.Raster("elevation", opacity=0.5).add_to(m)
    >>> m

    >>> m = ipyleaflet.Map()
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
        """Reproject GRASS raster, export to PNG, and compute bounding box."""
        super().__init__(name, title, use_region, saved_region, renderer, **kwargs)
        # Render overlay
        # By doing this here instead of in add_to, we avoid rendering
        # twice if added to multiple maps. This mimics the behavior
        # folium.raster_layers.ImageOverlay()
        self._filename, self._bounds = self._renderer.render_raster(name)

    def add_to(self, interactive_map):
        """Add raster to map object which is an instance of either
        folium.Map or ipyleaflet.Map"""
        if get_backend(interactive_map) == "folium":
            import folium  # pylint: disable=import-outside-toplevel

            # Overlay image on folium map
            image = folium.raster_layers.ImageOverlay(
                image=self._filename,
                bounds=self._bounds,
                name=self._title,
                **self._layer_kwargs,
            )
            image.add_to(interactive_map)
        else:
            import ipyleaflet  # pylint: disable=import-outside-toplevel

            # ImageOverlays don't work well with local files,
            # they need relative address and behavior differs
            # for notebooks and jupyterlab
            data = base64.b64encode(Path(self._filename).read_bytes()).decode("ascii")
            url = "data:image/png;base64," + data
            image = ipyleaflet.ImageOverlay(
                url=url, bounds=self._bounds, name=self._title, **self._layer_kwargs
            )
            interactive_map.add(image)


class Vector(Layer):
    """Adds vectors to a folium or ipyleaflet map.

    Basic Usage:
    >>> m = folium.Map()
    >>> gj.Vector("roadsmajor").add_to(m)
    >>> m

    >>> m = ipyleaflet.Map()
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
        """Reproject GRASS vector and export to GeoJSON."""
        super().__init__(name, title, use_region, saved_region, renderer, **kwargs)
        self._filename = self._renderer.render_vector(name)

    def add_to(self, interactive_map):
        """Add vector to map"""
        if get_backend(interactive_map) == "folium":
            import folium  # pylint: disable=import-outside-toplevel

            folium.GeoJson(
                str(self._filename), name=self._title, **self._layer_kwargs
            ).add_to(interactive_map)
        else:
            import ipyleaflet  # pylint: disable=import-outside-toplevel

            with open(self._filename, "r", encoding="utf-8") as file:
                data = json.load(file)
            # allow using opacity directly to keep interface
            # consistent for both backends
            if "opacity" in self._layer_kwargs:
                opacity = self._layer_kwargs.pop("opacity")
                if "style" in self._layer_kwargs:
                    self._layer_kwargs["style"]["opacity"] = opacity
                else:
                    self._layer_kwargs["style"] = {"opacity": opacity}
            geo_json = ipyleaflet.GeoJSON(
                data=data, name=self._title, **self._layer_kwargs
            )
            interactive_map.add(geo_json)


class InteractiveMap:
    """This class creates interactive GRASS maps with folium or ipyleaflet.

    Basic Usage:

    >>> m = InteractiveMap()
    >>> m.add_vector("streams")
    >>> m.add_raster("elevation")
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
        map_backend=None,
    ):
        """Creates a blank folium/ipyleaflet map centered on g.region.

        If map_backend is not specified, InteractiveMap tries to import
        ipyleaflet first, then folium if it fails. The backend can be
        specified explicitely with valid values "folium" and "ipyleaflet" .

        In case of folium backend, tiles parameter is passed directly
        to folium.Map() which supports several built-in tilesets
        (including "OpenStreetMap", "Stamen Toner", "Stamen Terrain",
        "Stamen Watercolor", "Mapbox Bright", "Mapbox Control Room", "CartoDB positron",
        "CartoDB dark_matter") as well as custom tileset URL (i.e.
        "http://{s}.yourtiles.com/{z}/{x}/{y}.png"). For more information, visit
        folium documentation:
        https://python-visualization.github.io/folium/modules.html
        In case of ipyleaflet, only the tileset name and not the URL is
        currently supported.

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
        :param str map_backend: "ipyleaflet" or "folium" or None
        """
        self._ipyleaflet = None
        self._folium = None

        def _import_folium(error):
            try:
                import folium  # pylint: disable=import-outside-toplevel

                return folium
            except ImportError as err:
                if error:
                    raise err
                return None

        def _import_ipyleaflet(error):
            try:
                import ipyleaflet  # pylint: disable=import-outside-toplevel

                return ipyleaflet
            except ImportError as err:
                if error:
                    raise err
                return None

        if not map_backend:
            self._ipyleaflet = _import_ipyleaflet(error=False)
            if not self._ipyleaflet:
                self._folium = _import_folium(error=False)
            if not (self._ipyleaflet or self._folium):
                raise ImportError(_("Neither ipyleaflet nor folium found."))
        elif map_backend == "folium":
            self._folium = _import_folium(error=True)

        elif map_backend == "ipyleaflet":
            self._ipyleaflet = _import_ipyleaflet(error=True)
        else:
            raise ValueError(_("Invalid map backend, use 'folium' or 'ipyleaflet'"))

        if self._ipyleaflet:
            import ipywidgets as widgets  # pylint: disable=import-outside-toplevel
            import xyzservices  # pylint: disable=import-outside-toplevel

        # Store height and width
        self.width = width
        self.height = height

        if self._ipyleaflet:
            basemap = xyzservices.providers.query_name(tiles)
            if API_key and basemap.get("accessToken"):
                basemap["accessToken"] = API_key
            layout = widgets.Layout(width=f"{width}px", height=f"{height}px")
            self.map = self._ipyleaflet.Map(
                basemap=basemap, layout=layout, scroll_wheel_zoom=True
            )

        else:
            self.map = self._folium.Map(
                width=self.width,
                height=self.height,
                tiles=tiles,
                API_key=API_key,  # pylint: disable=invalid-name
            )
        # Set LayerControl default
        self.layer_control_object = None
        self.region_rectangle = None

        self._renderer = ReprojectionRenderer(
            use_region=use_region, saved_region=saved_region
        )

    def add_vector(self, name, title=None, **kwargs):
        """Imports vector into temporary WGS84 location, re-formats to a GeoJSON and
        adds to map.

        :param str name: name of vector to be added to map;
                         positional-only parameter
        :param str title: vector name for layer control
        :**kwargs: keyword arguments passed to GeoJSON overlay
        """
        Vector(name, title=title, renderer=self._renderer, **kwargs).add_to(self.map)

    def add_raster(self, name, title=None, **kwargs):
        """Imports raster into temporary WGS84 location,
        exports as png and overlays on a map.

        Color table for the raster can be modified with `r.colors` before calling
        this function.

        .. note:: This will only work if the raster is located in the current mapset.
        To change the color table of a raster located outside the current mapset,
        switch to that mapset with `g.mapset`, modify the color table with `r.color`
        then switch back to the initial mapset and run this function.

        :param str name: name of raster to add to display; positional-only parameter
        :param str title: raster name for layer control
        :**kwargs: keyword arguments passed to image overlay
        """
        Raster(name, title=title, renderer=self._renderer, **kwargs).add_to(self.map)

    def add_layer_control(self, **kwargs):
        """Add layer control to display.

        A Layer Control is added by default. Call this function to customize
        layer control object. Accepts keyword arguments to be passed to leaflet
        layer control object"""

        if self._folium:
            self.layer_control_object = self._folium.LayerControl(**kwargs)
        else:
            self.layer_control_object = self._ipyleaflet.LayersControl(**kwargs)

    def draw_computational_region(self):
        """
        Allow users to draw the computational region and modify it.
        """
        import ipywidgets as widgets  # pylint: disable=import-outside-toplevel

        region_mode_button = widgets.ToggleButton(
            icon="square-o",
            description="",
            value=False,
            tooltip="Click to show and edit computational region",
            layout=widgets.Layout(width="40px", margin="0px 0px 0px 0px"),
        )

        save_button = widgets.Button(
            description="Update region",
            tooltip="Click to update region",
            disabled=True,
        )
        bottom_output_widget = widgets.Output(
            layout={
                "width": "100%",
                "max_height": "300px",
                "overflow": "auto",
                "display": "none",
            }
        )

        changed_region = {}
        save_button_control = None

        def update_output(region):
            with bottom_output_widget:
                bottom_output_widget.clear_output()
                print(
                    _(
                        "Region changed to: n={n}, s={s}, e={e}, w={w} "
                        "nsres={nsres} ewres={ewres}"
                    ).format(**region)
                )

        def on_rectangle_change(value):
            save_button.disabled = False
            bottom_output_widget.layout.display = "none"
            latlon_bounds = value["new"][0]
            changed_region["north"] = latlon_bounds[2]["lat"]
            changed_region["south"] = latlon_bounds[0]["lat"]
            changed_region["east"] = latlon_bounds[2]["lng"]
            changed_region["west"] = latlon_bounds[0]["lng"]

        def toggle_region_mode(change):
            nonlocal save_button_control

            if change["new"]:
                region_bounds = get_region_bounds_latlon()
                self.region_rectangle = self._ipyleaflet.Rectangle(
                    bounds=region_bounds,
                    color="red",
                    fill_color="red",
                    fill_opacity=0.5,
                    draggable=True,
                    transform=True,
                    rotation=False,
                    name="Computational region",
                )
                self.region_rectangle.observe(on_rectangle_change, names="locations")
                self.map.fit_bounds(region_bounds)
                self.map.add(self.region_rectangle)

                save_button_control = self._ipyleaflet.WidgetControl(
                    widget=save_button, position="topright"
                )
                self.map.add(save_button_control)
            else:
                if self.region_rectangle:
                    self.region_rectangle.transform = False
                    self.map.remove(self.region_rectangle)
                    self.region_rectangle = None

                save_button.disabled = True

                if save_button_control:
                    self.map.remove(save_button_control)
                bottom_output_widget.layout.display = "none"

        def save_region(_change):
            from_proj = "+proj=longlat +datum=WGS84 +no_defs"
            to_proj = get_location_proj_string()
            reprojected_region = reproject_region(changed_region, from_proj, to_proj)
            new = update_region(reprojected_region)
            bottom_output_widget.layout.display = "block"
            update_output(new)

        region_mode_button.observe(toggle_region_mode, names="value")
        save_button.on_click(save_region)

        region_mode_control = self._ipyleaflet.WidgetControl(
            widget=region_mode_button, position="topright"
        )
        self.map.add(region_mode_control)

        output_control = self._ipyleaflet.WidgetControl(
            widget=bottom_output_widget, position="bottomleft"
        )
        self.map.add(output_control)

    def show(self):
        """This function returns a folium figure or ipyleaflet map object
        with a GRASS raster and/or vector overlaid on a basemap.

        If map has layer control enabled, additional layers cannot be
        added after calling show()."""
        if self._ipyleaflet:
            self.draw_computational_region()
        self.map.fit_bounds(self._renderer.get_bbox())

        if not self.layer_control_object:
            self.add_layer_control()

        # folium
        if self._folium:
            self.map.add_child(self.layer_control_object)
            fig = self._folium.Figure(width=self.width, height=self.height)
            fig.add_child(self.map)

            return fig

        # ipyleaflet
        self.map.add(self.layer_control_object)
        return self.map

    def save(self, filename):
        """Save map as an html map.

        :param str filename: name of html file
        """
        self.map.save(filename)
