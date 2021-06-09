import os
import grass.script as gs
import grass.script.setup as gsetup


def init(gisbase, grassdata_filepath, location, mapset):
    # Create a GRASS GIS session. This is a user set parameter
    return gsetup.init(gisbase, grassdata_filepath, location, mapset)

def use_notebook_defaults():
    # We want functions to raise exceptions and see standard output of the modules in the notebook.
    gs.set_raise_on_error(True)
    gs.set_capture_stderr(True)
    # Simply overwrite existing maps like we overwrite Python variable values.
    os.environ['GRASS_OVERWRITE'] = '1'

def display_settings(font='sans', render_immediate='cairo', render_file_read='TRUE', legend_file='legend.txt' ):
    # Enable map rendering in a notebook.
    os.environ['GRASS_FONT'] = font
    # Set display modules to render into a file (named map.png by default).
    os.environ['GRASS_RENDER_IMMEDIATE'] = render_immediate
    os.environ['GRASS_RENDER_FILE_READ'] = render_file_read
    os.environ['GRASS_LEGEND_FILE'] = legend_file
