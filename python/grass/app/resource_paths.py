"""Paths to resources and and other GRASS properties, configured during build

(C) 2025 by Nicklas Larsson and the GRASS Development Team

SPDX-License-Identifier: GPL-2.0-or-later


This is not a stable part of the API. Use at your own risk.

The "@...@" variables are being substituted during build process

"""

GRASS_VERSION = "@GRASS_VERSION_NUMBER@"
GRASS_VERSION_MAJOR = "@GRASS_VERSION_MAJOR@"
GRASS_VERSION_MINOR = "@GRASS_VERSION_MINOR@"
LD_LIBRARY_PATH_VAR = "@LD_LIBRARY_PATH_VAR@"
CONFIG_PROJSHARE = "@CONFIG_PROJSHARE@"
GRASS_EXE_NAME = "@START_UP@"
GRASS_VERSION_GIT = "@GRASS_VERSION_GIT@"

GRASS_PREFIX = "@GRASS_PREFIX@"
GISBASE = "@GISBASE_INSTALL_PATH@"
