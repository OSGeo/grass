import os
import tempfile
import weakref
import shutil

import grass.script as gs

from .utils import save_gif
from .map import Map


class BaseSeriesMap:
    def __init__(
        self, width=None, height=None, env=None, use_region=False, saved_region=None
    ):
        # Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()

        self._base_layer_calls = []
        self._layers_rendered = False
        self._width = width
        self._height = height
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

    def __getattr__(self, name):
        # Check to make sure format is correct
        if not name.startswith("d_"):
            raise AttributeError(_("Module must begin with 'd_'"))
        # Reformat string
        grass_module = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(grass_module):
            raise AttributeError(_("Cannot find GRASS module {}").format(grass_module))

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
        base_file = os.path.join(self._tmpdir.name, random_name_base)
        img = Map(
            width=self._width,
            height=self._height,
            filename=base_file,
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

    def show(
        self,
        slider_width=None,
        options=None,
        value=None,
        description=None,
        max=None,
        label=None,
    ):
        """Create interactive timeline slider.

        param str slider_width: width of datetime selection slider

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

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options,
            value,
            description,
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
            max=max,
            step=1,
            description="Press play",
            disabled=False,
        )
        out_img = widgets.Image(value=b"", format="png")

        def change_slider(change):
            if isinstance(slider.options[0], tuple):
                slider.value = slider.options[change.new][1]
            else:
                slider.value = slider.options[change.new]

        play.observe(change_slider, names="value")

        # Display image associated with datetime
        def change_image(date):
            # Look up layer name for date
            filename = self._date_filename_dict[date]
            with open(filename, "rb") as rfile:
                out_img.value = rfile.read()

        # Return interact widget with image and slider
        widgets.interactive_output(change_image, {label: slider})
        layout = widgets.Layout(
            width="100%", display="inline-flex", flex_flow="row wrap"
        )
        return widgets.HBox([play, slider, out_img], layout=layout)

    def save(
        self,
        filename,
        save_files,
        labels,
        duration=500,
        label=True,
        font=None,
        text_size=12,
        text_color="gray",
    ):
        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        input_files = []
        for date in self._dates:
            input_files.append(self._date_filename_dict[date])

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
