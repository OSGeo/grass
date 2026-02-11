"""Utility functions and classes for the temporal framework.

(C) 2025 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: GRASS Development Team
"""

# Re-export from canonical location for backward compatibility
from grass.script.utils import TemporalJSONEncoder

__all__ = ["TemporalJSONEncoder"]
