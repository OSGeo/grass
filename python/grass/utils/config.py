# MODULE:    grass.utils.config
#
# AUTHOR(S): Martin Landa <landa.martin gmail com>
#
# PURPOSE:   Collection of various helper general (non-GRASS) utilities
#
# COPYRIGHT: (C) 2024 Martin Landa, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Define GRASS config path"""

import os
import sys


class ConfigError(Exception):
    """Error related to config directory handling"""


def get_grass_config_dir():
    """Get configuration directory

    Determines path of GRASS GIS user configuration directory and creates
    it if it does not exist.

    Configuration directory is for example used for grass env file
    (the one which caries mapset settings from session to session).

    Raise ConfigError on failure

    :return str: path to configuration directory
    """
    windows = sys.platform.startswith("win")
    # major version
    try:
        grass_version_major = int(os.getenv("GRASS_VERSION").split(".")[0])
    except (AttributeError, ValueError, IndexError) as e:
        raise ConfigError("Unable to determine GRASS version: {}".format(e))

    if windows:
        grass_config_dirname = f"GRASS{grass_version_major}"
    else:
        grass_config_dirname = f".grass{grass_version_major}"

    if os.getenv("GRASS_CONFIG_DIR"):
        config_directory = os.path.join(
            os.getenv("GRASS_CONFIG_DIR"), grass_config_dirname
        )
    else:
        if windows:
            conf_var = "APPDATA"
        else:
            conf_var = "HOME"
        conf_path = os.getenv(conf_var)

        # this can happen with some strange settings
        if conf_path is None:
            raise ConfigError(
                "The {} variable is not set, ask your operating"
                " system support".format(conf_var)
            )
        if not os.path.exists(conf_path):
            raise ConfigError(
                "The {} variable points to directory which does"
                " not exist, ask your operating system support".format(conf_var)
            )

        config_directory = os.path.join(conf_path, grass_config_dirname)

    return config_directory
