"""Base class for SeriesMap and TimeSeriesMap"""

import os
import tempfile
import weakref
import shutil

import grass.script as gs

from .utils import save_gif
from .map import Map


class BaseSeriesMap:
    """
    Base class for SeriesMap and TimeSeriesMap
    """

    def __init__(
        self, width=None, height=None, env=None, use_region=False, saved_region=None
    ):
        """Creates an instance of the visualizations class.

        :param int width: width of map in pixels
        :param int height: height of map in pixels
        :param str env: environment
        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        """

        # Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()

        self._base_layer_calls = []
        self._layers_rendered = False
        self._layer_filename_dict = {}
        self._date_filename_dict = {}
        self._width = width
        self._height = height
        self._dates = None
        self._names = []
        self.base_file = None
        self.grass_module = None
        # Create a temporary directory for our PNG images
        # Resource managed by weakref.finalize.
        self._tmpdir = (
            # pylint: disable=consider-using-with
            tempfile.TemporaryDirectory()
        )

        def cleanup(tmpdir):
            tmpdir.cleanup()

        weakref.finalize(self, cleanup, self._tmpdir)

        # Handle regions in respective classes
        self.use_region = use_region
        self.saved_region = saved_region

    def __getattr__(self, name):
        """
        Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """
        # Check to make sure format is correct
        if not name.startswith("d_"):
            raise AttributeError(_("Module must begin with 'd_'"))
        # Reformat string
        self.grass_module = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(self.grass_module):
            raise AttributeError(
                _("Cannot find GRASS module {}").format(self.grass_module)
            )

        # Wrapper function is in respective classes

    def _render_baselayers(self, img):
        """Add collected baselayers to Map instance"""
        for grass_module, kwargs in self._base_layer_calls:
            img.run(grass_module, **kwargs)

    def render(self):
        """Renders image for each time-step in space-time dataset.

        Save PNGs to temporary directory. Must be run before creating a visualization
        (i.e. show or save). Can be time-consuming to run with large
        space-time datasets.
        """
        # Runtime error in respective classes

        # Make base image (background and baselayers)
        # Random name needed to avoid potential conflict with layer names
        random_name_base = gs.append_random("base", 8) + ".png"
        self.base_file = os.path.join(self._tmpdir.name, random_name_base)
        img = Map(
            width=self._width,
            height=self._height,
            filename=self.base_file,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        # We have to call d_erase to ensure the file is created. If there are no
        # base layers, then there is nothing to render in random_base_name
        img.d_erase()
        # Add baselayers
        self._render_baselayers(img)
        self._layers_rendered = True

    def show(self, slider_width=None, is_date=None):
        """Create interactive timeline slider.

        param str slider_width: width of datetime selection slider
        param list options: list of options for the slider
        param str value: initial value of the slider
        param str description: description of the slider
        param int max_value: maximum value of the slider
        param bool label: include date/time stamp on each frame

        The slider_width parameter sets the width of the slider in the output cell.
        It should be formatted as a percentage (%) between 0 and 100 of the cell width
        or in pixels (px). Values should be formatted as strings and include the "%"
        or "px" suffix. For example, slider_width="80%" or slider_width="500px".
        slider_width is passed to ipywidgets in ipywidgets.Layout(width=slider_width).
        """
        # Lazy Imports
        import ipywidgets as widgets  # pylint: disable=import-outside-toplevel

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        # Set default slider width
        if not slider_width:
            slider_width = "70%"

        if is_date:
            options = self._dates
            value = self._dates[0]
            description = _("Date/Time")
            max_value = len(self._dates) - 1
        else:
            lookup = list(zip(self._names, range(self._series_length)))
            options = lookup
            value = 0
            description = None
            max_value = self._series_length - 1

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options=options,
            value=value,
            description=description,
            disabled=False,
            continuous_update=True,
            orientation="horizontal",
            readout=True,
            layout=widgets.Layout(width=slider_width),
        )
        play = widgets.Play(
            interval=500,
            value=0,
            min=0,
            max=max_value,
            step=1,
            description="Press play",
            disabled=False,
        )
        out_img = widgets.Image(value=b"", format="png")

        def change_slider(change):
            if is_date:
                slider.value = slider.options[change.new]
            else:
                slider.value = slider.options[change.new][1]

        play.observe(change_slider, names="value")

        # Display image associated with datetime
        if is_date:

            def change_image(date):
                filename = self._date_filename_dict[date]
                with open(filename, "rb") as rfile:
                    out_img.value = rfile.read()

            widgets.interactive_output(change_image, {"date": slider})
        else:

            def change_image(index):
                filename = self._layer_filename_dict[index]
                with open(filename, "rb") as rfile:
                    out_img.value = rfile.read()

            widgets.interactive_output(change_image, {"index": slider})

        layout = widgets.Layout(
            width="100%", display="inline-flex", flex_flow="row wrap"
        )
        return widgets.HBox([play, slider, out_img], layout=layout)

    def save(
        self,
        filename,
        duration=500,
        label=True,
        font=None,
        text_size=12,
        text_color="gray",
        is_date=None,
    ):
        """
        Creates a GIF animation of rendered layers.

        Text color must be in a format accepted by PIL ImageColor module. For supported
        formats, visit:
        https://pillow.readthedocs.io/en/stable/reference/ImageColor.html#color-names

        param str filename: name of output GIF file
        param int duration: time to display each frame; milliseconds
        param bool label: include date/time stamp on each frame
        param str font: font file
        param int text_size: size of date/time text
        param str text_color: color to use for the text.
        param bool is_date: True if timeseriesmap, False if seriesmap
        """
        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        if is_date:
            input_files = []
            for date in self._dates:
                input_files.append(self._date_filename_dict[date])
            save_files = input_files
            labels = self._dates
        else:
            tmp_files = []
            for _, file in self._layer_filename_dict.items():
                tmp_files.append(file)
            save_files = tmp_files
            labels = self._names

        save_gif(
            save_files,
            filename,
            duration=duration,
            label=label,
            labels=labels,
            font=font,
            text_size=text_size,
            text_color=text_color,
        )

        # Display the GIF
        return filename
