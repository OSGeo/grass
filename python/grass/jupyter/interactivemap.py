#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#            Anna Petrasova <kratochanna AT gmail>
#            Riya Saxena <29riyasaxena AT gmail>
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

import os
import base64
import json
from pathlib import Path
from .reprojection_renderer import ReprojectionRenderer

from .utils import (
    get_region_bounds_latlon,
    reproject_region,
    update_region,
    get_location_proj_string,
    save_vector,
    get_region,
    query_raster,
    query_vector,
    reproject_latlon,
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

            with open(self._filename, encoding="utf-8") as file:
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
        self._ipywidgets = None

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

            self._ipywidgets = widgets
            import xyzservices  # pylint: disable=import-outside-toplevel

        # Store height and width
        self.width = int(width)
        self.height = int(height)
        self._controllers = {}

        # Store vector and raster name
        self.raster_name = []
        self.vector_name = []

        # Store Region
        self.region = None

        if self._ipyleaflet:
            basemap = xyzservices.providers.query_name(tiles)
            if API_key and basemap.get("accessToken"):
                basemap["accessToken"] = API_key
            layout = self._ipywidgets.Layout(width=f"{width}px", height=f"{height}px")
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
        self.vector_name.append(name)
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
        self.raster_name.append(name)
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

    def setup_drawing_interface(self):
        """Sets up the drawing interface for users
        to interactively draw and manage geometries on the map.

        This includes creating a toggle button to activate the drawing mode, and
        instantiating an InteractiveDrawController to handle the drawing functionality.
        """
        return self._create_toggle_button(
            icon="pencil",
            tooltip=_("Click to draw geometries"),
            controller_class=InteractiveDrawController,
        )

    def setup_computational_region_interface(self):
        """Sets up the interface for users to draw and
        modify the computational region on the map.

        This includes creating a toggle button to activate the
        region editing mode, and instantiating an InteractiveRegionController to
        handle the region selection and modification functionality.
        """
        return self._create_toggle_button(
            icon="square-o",
            tooltip=_("Click to show and edit computational region"),
            controller_class=InteractiveRegionController,
        )

    def setup_query_interface(self):
        """Sets up the query button interface.

        This includes creating a toggle button to activate the
        query mode, and instantiating an InteractiveQueryController to
        handle the user query.
        """
        return self._create_toggle_button(
            icon="info",
            tooltip=_("Click to query raster and vector maps"),
            controller_class=InteractiveQueryController,
        )

    def _create_toggle_button(self, icon, tooltip, controller_class):
        button = self._ipywidgets.ToggleButton(
            icon=icon,
            value=False,
            tooltip=tooltip,
            description="",
            # layout=self._ipywidgets.Layout(
            #     width="43px", margin="0px", #border="2px solid darkgrey"
            # ),
        )
        controller = controller_class(
            map_object=self.map,
            ipyleaflet=self._ipyleaflet,
            ipywidgets=self._ipywidgets,
            toggle_button=button,
            rasters=self.raster_name,
            vectors=self.vector_name,
            width=self.width,
        )
        self._controllers[button] = controller
        button.observe(self._toggle_mode, names="value")
        return button

    def _toggle_mode(self, change):
        if change["new"]:
            for button, controller in self._controllers.items():
                if button is not change["owner"]:
                    button.value = False
                    controller.deactivate()
            self._controllers[change["owner"]].activate()
        else:
            self._controllers[change["owner"]].deactivate()

    def show(self):
        """This function returns a folium figure or ipyleaflet map object
        with a GRASS raster and/or vector overlaid on a basemap.

        If map has layer control enabled, additional layers cannot be
        added after calling show()."""
        if self._ipyleaflet:
            toggle_buttons = [
                self.setup_query_interface(),
                self.setup_computational_region_interface(),
                self.setup_drawing_interface(),
            ]
            button_box = self._ipywidgets.HBox(
                toggle_buttons, layout=self._ipywidgets.Layout(width="150px")
            )
            self.map.add(
                self._ipyleaflet.WidgetControl(widget=button_box, position="topright")
            )

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


class InteractiveRegionController:
    """A controller for interactive region selection on a map.

    Attributes:
        map: The map object.
        region_rectangle: The rectangle representing the selected region.
        _ipyleaflet: The ipyleaflet module.
        _ipywidgets: The ipywidgets module.
        save_button: The button to save the selected region.
        bottom_output_widget: The output widget to display the selected region.
        changed_region (dict): The dictionary to store the changed region.
    """

    def __init__(
        self, map_object, ipyleaflet, ipywidgets, **kwargs
    ):  # pylint: disable=unused-argument
        """Initializes the InteractiveRegionController.

        :param map_object: The map object.
        :param ipyleaflet: The ipyleaflet module.
        :param ipywidgets: The ipywidgets module.
        """
        self.map = map_object
        self.region_rectangle = None
        self._ipyleaflet = ipyleaflet
        self._ipywidgets = ipywidgets

        self.save_button = self._ipywidgets.Button(
            description="Update region",
            tooltip="Click to update region",
            disabled=True,
        )
        self.bottom_output_widget = self._ipywidgets.Output(
            layout={
                "width": "100%",
                "max_height": "300px",
                "overflow": "auto",
                "display": "none",
            }
        )
        self.changed_region = {}
        self.save_button_control = None
        self.save_button.on_click(self._save_region)

        output_control = self._ipyleaflet.WidgetControl(
            widget=self.bottom_output_widget, position="bottomleft"
        )
        self.map.add(output_control)

    def _update_output(self, region):
        """Updates the output widget with the selected region.

        :param dict region: The selected region.
        """
        with self.bottom_output_widget:
            self.bottom_output_widget.clear_output()
            print(
                _(
                    "Region changed to: n={n}, s={s}, e={e}, w={w} "
                    "nsres={nsres} ewres={ewres}"
                ).format(**region)
            )

    def _on_rectangle_change(self, value):
        """Handles the change event of the rectangle.

        :param dict value: The changed value.
        """
        self.save_button.disabled = False
        self.bottom_output_widget.layout.display = "none"
        latlon_bounds = value["new"][0]
        self.changed_region["north"] = latlon_bounds[2]["lat"]
        self.changed_region["south"] = latlon_bounds[0]["lat"]
        self.changed_region["east"] = latlon_bounds[2]["lng"]
        self.changed_region["west"] = latlon_bounds[0]["lng"]

    def activate(self):
        """Activates the interactive region selection."""
        region_bounds = get_region_bounds_latlon()
        self.region_rectangle = self._ipyleaflet.Rectangle(
            bounds=region_bounds,
            color="red",
            fill_opacity=0,
            opacity=0.5,
            draggable=True,
            transform=True,
            rotation=False,
            name="Computational region",
        )
        self.region_rectangle.observe(self._on_rectangle_change, names="locations")
        self.map.fit_bounds(region_bounds)
        self.map.add(self.region_rectangle)

        self.save_button_control = self._ipyleaflet.WidgetControl(
            widget=self.save_button, position="topright"
        )
        self.map.add(self.save_button_control)

    def deactivate(self):
        """Deactivates the interactive region selection."""
        if self.region_rectangle:
            self.region_rectangle.transform = False
            self.map.remove(self.region_rectangle)
            self.region_rectangle = None

        if (
            hasattr(self, "save_button_control")
            and self.save_button_control in self.map.controls
        ):
            self.map.remove(self.save_button_control)

        self.save_button.disabled = True
        self.bottom_output_widget.layout.display = "none"

    def _save_region(self, _change):
        """Saves the selected region.

        :param _change:Not used.
        """
        from_proj = "+proj=longlat +datum=WGS84 +no_defs"
        to_proj = get_location_proj_string()
        reprojected_region = reproject_region(self.changed_region, from_proj, to_proj)
        new = update_region(reprojected_region)
        self.bottom_output_widget.layout.display = "block"
        self._update_output(new)


class InteractiveDrawController:
    """A controller for interactive drawing on a map.

    Attributes:
        map: The map object.
        _ipyleaflet: The ipyleaflet module.
        draw_control: The draw control.
        drawn_geometries: The list of drawn geometries.
        self.vector_layers: List of vector layers
        geo_json_layers: The dictionary of GeoJSON layers.
        save_button_control: The save button control.
        toggle_button: The toggle button activating/deactivating drawing.
    """

    def __init__(
        self, map_object, ipyleaflet, ipywidgets, toggle_button, vectors, **kwargs
    ):  # pylint: disable=unused-argument
        """Initializes the InteractiveDrawController.

        :param map_object: The map object.
        :param ipyleaflet: The ipyleaflet module.
        :param ipywidgets: The ipywidgets module.
        :param toggle_button: The toggle button activating/deactivating drawing.
        :param vectors: List of vector layers.
        """
        self.map = map_object
        self._ipyleaflet = ipyleaflet
        self._ipywidgets = ipywidgets
        self.toggle_button = toggle_button
        self.vector_layers = vectors
        self.draw_control = self._ipyleaflet.DrawControl(edit=False, remove=False)
        self.drawn_geometries = []
        self.geo_json_layers = {}
        self.save_button_control = None

        self.name_input = self._ipywidgets.Text(
            description=_("New vector map name:"),
            style={"description_width": "initial"},
            layout=self._ipywidgets.Layout(width="80%", margin="1px 1px 1px 5px"),
        )

        self.save_button = self._ipywidgets.Button(
            description=_("Save"),
            layout=self._ipywidgets.Layout(width="20%", margin="1px 1px 1px 1px"),
        )

        self.save_button.on_click(self._save_geometries)

    def activate(self):
        """Activates the interactive drawing."""
        self.map.add_control(self.draw_control)
        self.draw_control.on_draw(self._handle_draw)
        self._show_interface()

    def deactivate(self):
        """Deactivates the interactive drawing."""
        self.draw_control.clear()
        if self.draw_control in self.map.controls:
            self.map.remove(self.draw_control)
        self.drawn_geometries.clear()
        self._hide_interface()

    def _handle_draw(self, _, action, geo_json):
        """Handles the draw event.

        :param str action: The action type.
        :param dict geo_json: The GeoJSON data.
        """
        if action == "created":
            self.drawn_geometries.append(geo_json)
            print(f"Geometry created: {geo_json}")

    def _show_interface(self):
        """Shows the interface for saving the drawn geometries."""
        hbox_layout = self._ipywidgets.Layout(
            display="flex",
            flex_flow="row",
            align_items="stretch",
            width="300px",
            justify_content="space-between",
        )
        self.name_input.value = ""
        self.save_button_control = self._ipyleaflet.WidgetControl(
            widget=self._ipywidgets.HBox(
                [self.name_input, self.save_button], layout=hbox_layout
            ),
            position="topright",
        )

        self.map.add_control(self.save_button_control)

    def _hide_interface(self):
        """Hides the interface for saving the drawn geometries."""
        if self.save_button_control:
            self.map.remove_control(self.save_button_control)
            self.save_button_control = None

    def _save_geometries(self, _b):
        """Saves the drawn geometries.

        :param _b: Not used.
        """
        name = self.name_input.value
        if name and self.drawn_geometries:
            for geometry in self.drawn_geometries:
                geometry["properties"]["name"] = name
            geo_json = {
                "type": "FeatureCollection",
                "features": self.drawn_geometries,
            }
            save_vector(name, geo_json)
            geo_json_layer = self._ipyleaflet.GeoJSON(data=geo_json, name=name)
            self.geo_json_layers[name] = geo_json_layer
            self.vector_layers.append(name)
            self.map.add_layer(geo_json_layer)
            self.deactivate()
            self.toggle_button.value = False


class InteractiveQueryController:
    """A controller for interactive querying on a map.

    Attributes:
        map: The ipyleaflet.Map object.
        _ipyleaflet: The ipyleaflet module.
        _ipywidgets: The ipywidgets module.
        raster_name: The name of the raster layer.
        vector_name: The name of the vector layer.
        width: The width of the map as an int.
        query_control: The query control.

    """

    def __init__(
        self, map_object, ipyleaflet, ipywidgets, rasters, vectors, width, **kwargs
    ):  # pylint: disable=unused-argument
        """Initializes the InteractiveQueryController.

        :param map: The map object.
        :param ipyleaflet: The ipyleaflet module.
        :param ipywidgets: The ipywidgets module.
        """
        self.map = map_object
        self._ipyleaflet = ipyleaflet
        self._ipywidgets = ipywidgets
        self.raster_name = rasters
        self.vector_name = vectors
        self.width = width
        self.query_control = None

    def activate(self):
        """Activates the interactive querying."""
        self.map.on_interaction(self.handle_interaction)
        self.map.default_style = {"cursor": "crosshair"}

    def deactivate(self):
        """Deactivates the interactive querying."""
        self.map.default_style = {"cursor": "default"}
        self.map.on_interaction(self.handle_interaction, remove=True)
        self.clear_popups()

    def handle_interaction(self, **kwargs):
        """Handles the map interaction event.

        :param kwargs: The event arguments.
        """
        if kwargs.get("type") != "click":
            return

        lonlat = kwargs.get("coordinates")
        reprojected_coordinates = reproject_latlon(lonlat)
        raster_output = self.query_raster(reprojected_coordinates)
        vector_output = self.query_vector(reprojected_coordinates)
        self.show_popup(lonlat, raster_output + vector_output)

    def query_raster(self, coordinates):
        """Queries the raster layer.

        :param coordinates: The coordinates.
        :return: The raster output.
        """
        return query_raster(coordinates, self.raster_name)

    def query_vector(self, coordinates):
        """Queries the vector layer.

        :param coordinates: The coordinates.
        :return: The vector output.
        """
        region = get_region(env=os.environ.copy())
        return query_vector(
            coordinates,
            self.vector_name,
            10.0 * ((region["east"] - region["west"]) / self.width),
        )

    def show_popup(self, lonlat, message_content):
        """Shows a popup with the query result.

        :param lonlat: The latitude and longitude coordinates.
        :param message_content: The message content.
        """
        scrollable_container = self._ipywidgets.HTML(
            value=(
                "<div style='max-height: 300px; max-width: 300px; "
                "overflow-y: auto; overflow-x: auto;'>"
                f"{message_content}"
                "</div>"
            )
        )

        popup = self._ipyleaflet.Popup(
            location=lonlat,
            child=scrollable_container,
            close_button=False,
            auto_close=True,
            close_on_escape_key=False,
        )
        self.map.add(popup)

    def clear_popups(self):
        """Clears the popups."""
        for item in reversed(list(self.map.layers)):
            if isinstance(item, self._ipyleaflet.Popup):
                self.map.remove(item)
