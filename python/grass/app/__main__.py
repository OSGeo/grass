##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Minimal file for package execution for CLI interface
#
# SPDX-FileCopyrightText: 2025 Vaclav Petras
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
##############################################################################

"""Minimal file for package execution for low-level CLI interface

This is not a stable part of the API. Contact developers before using it.
"""

import sys
from .cli import main

sys.exit(main())

