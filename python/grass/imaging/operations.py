"""
Image non-geospatial operations and manipulations

Note: Functions in this module are experimental and are not considered
a stable API, i.e. may change in future releases of GRASS GIS.

It heavily relies on PIL but unlike PIL, the functions operate on
files instead of PIL Image objects (which are used internally).
These functions are convenient for post-processing outputs from GRASS
modules, e.g. after rendering. However, if you have multiple operations
you may want to consider using PIL directly for efficiency (to avoid
writing and reading from the files).

Usage
=====

Use keyword arguments for all parameters other than those for input,
output, and format. All function provide reasonable defaults if possible,
but note that they may not be applicable to you case or when developing
a general tool.

>>> import grass.imaging.operations as iop

>>> # replace white color in the image by 100% transparency
>>> iop.change_rbg_to_transparent("map.png", color=(255, 255, 255))

>>> # crop the image in place
>>> iop.crop_image("map.png")

>>> # create a new image with inverted colors of the original image
>>> iop.invert_image_colors("map.png", "map_inverted.png")

>>> # create a thumbnail of the original image
>>> iop.thumbnail_image("map.png", "map_thumbnail.png", size=(64, 64))

Error handling
==============

When PIL or a required submodule is not available, a RuntimeError
exception is raised with a message mentioning the missing dependency.
Additionally, any of the exceptions raised by PIL may be raised too,
for example, when the file is not found.

Authors, copyright and license
==============================

(C) 2018 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>
"""


# import similar to what is in visvis
try:
    import PIL
    from PIL import Image
    PILLOW = True
    try:
        from PIL import PILLOW_VERSION  # pylint: disable=unused-import
    except ImportError:
        PILLOW = False
    try:
        import PIL.ImageOps as ImageOps
    except ImportError:
        ImageOps = None
except ImportError:
    PIL = None


def crop_image(input_file, output_file=None, format=None):
    """Crop to non-zero area of the image

    :param input_file: Name of the file to manipulate
    :param output_file: filename for the new file (same as input by default)
    :param format: format to be used new file (if different from extension)
    """
    if PIL is None:
        raise RuntimeError(_("Install PIL or Pillow to use this function"))
    if not output_file:
        output_file = input_file
    img = Image.open(input_file)
    box = img.getbbox()
    cropped_image = img.crop(box)
    cropped_image.save(output_file, format)


def thumbnail_image(input_file, output_file=None, size=(200, 200),
                    format=None):
    """Create a thumbnail of an image

    The image aspect ratio is kept and its height and width are adjusted
    accordingly to fit the ``size`` parameter.

    :param input_file: Name of the file to manipulate
    :param size: Size of the new image in pixels as tuple
    :param output_file: filename for the new file (same as input by default)
    :param format: format to be used new file (if different from extension)
    """
    if PIL is None:
        raise RuntimeError(_("Install PIL or Pillow to use this function"))
    if not output_file:
        output_file = input_file
    img = Image.open(input_file)
    img.thumbnail(size, Image.ANTIALIAS)
    img.save(output_file, format)


def change_rbg_to_transparent(input_file, output_file=None, color='white',
                              alpha=0, format=None):
    """Make a specified RGB color in the image transparent

    The color is specified as a RGB tuple (triplet) or string 'white'
    or 'black'. Note that GRASS color names are not supported.
    The white (255, 255, 255) is replaced by default but each application
    is encouraged to consider color to use and explicitly specify it.

    :param input_file: Name of the file to manipulate
    :param color: Color to be replaced by transparency (tuple of three ints)
    :param alpha: Level of opacity (0 fully transparent, 255 fully opaque)
    :param output_file: filename for the new file (same as input by default)
    :param format: format to be used new file (if different from extension)
    """
    if PIL is None:
        raise RuntimeError(_("Install PIL or Pillow to use this function"))
    if color == 'white':
        rgb = (255, 255, 255)
    elif color == 'black':
        rgb = (0, 0, 0)
    else:
        rgb = color  # pylint: disable=redefined-variable-type
    if not output_file:
        output_file = input_file
    img = Image.open(input_file)
    img = img.convert("RGBA")
    old_data = img.getdata()
    new_data = []
    for item in old_data:
        if item[0] == rgb[0] and item[1] == rgb[1] and item[2] == rgb[2]:
            new_data.append((rgb[0], rgb[1], rgb[2], alpha))
        else:
            new_data.append(item)
    img.putdata(new_data)
    img.save(output_file, format)


def invert_image_colors(input_file, output_file=None, format=None):
    """Invert colors in the image

    The alpha channel, if present, is untouched by this function.

    :param input_file: Name of the file to manipulate
    :param output_file: filename for the new file (same as input by default)
    :param format: format to be used new file (if different from extension)
    """
    if PIL is None:
        raise RuntimeError(_("Install PIL or Pillow to use this function"))
    if ImageOps is None:
        raise RuntimeError(_("Install a newer version of PIL or Pillow to"
                             " use this function (missing ImageOps module)"))
    if not output_file:
        output_file = input_file
    original_img = Image.open(input_file)
    # according to documentation (3.0.x) the module can work only on RGB
    # so we need to specifically take care of transparency if present
    if original_img.mode == 'RGBA':
        # split into bands
        red1, green1, blue1, alpha = original_img.split()
        rgb_img = Image.merge('RGB', (red1, green1, blue1))
        # invert RGB
        inverted_rgb_img = ImageOps.invert(rgb_img)
        # put back the original alpha
        red2, green2, blue2 = inverted_rgb_img.split()
        new_image = Image.merge('RGBA', (red2, green2, blue2, alpha))
    else:
        new_image = ImageOps.invert(original_img)
    new_image.save(output_file, format)
