"""
This module imports the gui_tk or gui_wx module.
"""

import os

if __package__:
    from .gui_wx import *
else:
    from gui_wx import *
