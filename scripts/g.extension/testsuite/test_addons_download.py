"""
TEST:      test_addons_download.py

AUTHOR(S): Stefan Blumentrath <stefan dot blumentrath at nina dot no)
           based on test_addons_modules.py by
           Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test for g.extension individual modules/extensions download

COPYRIGHT: (C) 2020 Stefan Blumentrath, Vaclav Petras,
           and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import sys
import unittest

from pathlib import Path

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import silent_rmtree

ms_windows = sys.platform == "win32"


class TestModuleDownloadFromDifferentSources(TestCase):

    # MS Windows install function requires absolute paths
    install_prefix = Path("gextension_test_install_path").absolute()

    files = [
        install_prefix.joinpath("scripts", "r.example.plus"),
        install_prefix.joinpath("docs", "html", "r.example.plus.html"),
    ]

    def setUp(self):
        """Make sure we are not dealing with some old files"""
        if self.install_prefix.exists():
            files = list(path.name for path in self.install_prefix.iterdir())
            if files:
                RuntimeError(
                    "Install prefix path '{}' contains files {}".format(
                        str(self.install_prefix), files
                    )
                )

    def tearDown(self):
        """Remove created files"""
        silent_rmtree(str(self.install_prefix))

    @unittest.skipIf(ms_windows, "currently not supported on MS Windows")
    def test_github_install(self):
        """Test installing extension from github"""

        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://github.com/wenzeslaus/r.example.plus",
            prefix=str(self.install_prefix),
        )

        # Modules with non-standard branch would be good for testing...
        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://github.com/wenzeslaus/r.example.plus",
            prefix=str(self.install_prefix),
        )

        for file in self.files:
            self.assertFileExists(file)

    @unittest.skipIf(ms_windows, "currently not supported on MS Windows")
    def test_gitlab_install(self):
        """Test installing extension from gitlab"""
        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://gitlab.com/vpetras/r.example.plus",
            prefix=str(self.install_prefix),
        )

        for file in self.files:
            self.assertFileExists(file)

    @unittest.skipIf(ms_windows, "currently not supported on MS Windows")
    def test_bitbucket_install(self):
        """Test installing extension from bitbucket"""
        files = [
            self.install_prefix.joinpath("scripts", "r.sim.stats"),
            self.install_prefix.joinpath("docs", "html", "r.sim.stats.html"),
        ]
        self.assertModule(
            "g.extension",
            extension="r.sim.stats",
            url="https://bitbucket.org/lrntct/r.sim.stats",
            prefix=str(self.install_prefix),
        )

        for file in files:
            self.assertFileExists(file)

    def test_github_install_official(self):
        """Test installing C-extension from official addons repository"""
        files = [
            self.install_prefix.joinpath("bin", "r.gdd"),
            self.install_prefix.joinpath("docs", "html", "r.gdd.html"),
        ]
        self.assertModule(
            "g.extension", extension="r.gdd", prefix=str(self.install_prefix)
        )

        for file in files:
            self.assertFileExists(file)

    def test_github_install_official_multimodule(self):
        """Test installing multi-module extension from official addons repository"""
        files = [
            self.install_prefix.joinpath("scripts", "i.sentinel.parallel.download"),
            self.install_prefix.joinpath(
                "docs", "html", "i.sentinel.parallel.download.html"
            ),
            self.install_prefix.joinpath("scripts", "i.sentinel.import"),
            self.install_prefix.joinpath("docs", "html", "i.sentinel.import.html"),
        ]
        self.assertModule(
            "g.extension", extension="i.sentinel", prefix=str(self.install_prefix)
        )

        for file in files:
            self.assertFileExists(file)
            if not file.suffix == "html":
                self.assertModule(str(file), help=True)


if __name__ == "__main__":
    test()
