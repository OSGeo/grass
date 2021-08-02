"""
This module imports the gui_tk or gui_wx module.
"""

import os

projpicker_gui_env = "PROJPICKER_GUI"

gui = os.environ.get(projpicker_gui_env, "wx")

# https://stackoverflow.com/a/49480246/16079666
if __package__:
    if gui == "tk":
        from .gui_tk import *
    else:
        try:
            from .gui_wx import *
        except:
            from .gui_tk import *
else:
    if gui == "tk":
        from gui_tk import *
    else:
        try:
            from gui_wx import *
        except:
            from gui_tk import *
