##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Minimal file for package execution for CLI interface
#
# COPYRIGHT: (C) 2025 Vaclav Petras and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

"""Minimal file for package execution for low-level CLI interface

This is not a stable part of the API. Contact developers before using it.
"""

import sys
from .cli import main

sys.exit(main())
