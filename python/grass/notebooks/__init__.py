# MODULE:    grass.notebooks
#
# AUTHOR(S): Linda Karlovska <linda.karlovska seznam cz>
#
# PURPOSE:   Tools for managing Jupyter Notebooks within GRASS
#
# COPYRIGHT: (C) 2025 Linda Karlovska, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""
Tools for managing Jupyter Notebooks within GRASS

This module provides functionality for:
- Starting and stopping local Jupyter Notebook servers inside a GRASS GIS session
- Managing notebook directories linked to specific GRASS mapsets
- Creating default notebook templates for users
- Supporting integration with the GUI (e.g., wxGUI) and other tools

Unlike `grass.jupyter`, which allows Jupyter to access GRASS environments,
this module is focused on running Jupyter from within GRASS.

Example use case:
    - A user opens a panel in the GRASS that launches a Jupyter server
      and opens the associated notebook directory for the current mapset.

.. versionadded:: 8.5

"""

from .launcher import NotebookServerManager
from .directory import NotebookDirectoryManager

__all__ = [
    "Directory",
    "Launcher",
    "NotebookDirectoryManager",
    "NotebookServerManager",
]
