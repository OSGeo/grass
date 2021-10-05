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

import os
from importlib.machinery import SourceFileLoader
from distutils.spawn import find_executable

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.utils import silent_rmtree


class TestModuleDownloadFromDifferentSources(TestCase):

    # MS Windows install function requires absolute paths
    install_prefix = os.path.abspath("gextension_test_install_path")

    files = [
        os.path.join(install_prefix, "scripts", "r.example.plus"),
        os.path.join(install_prefix, "docs", "html", "r.example.plus.html"),
    ]

    def setUp(self):
        """Make sure we are not dealing with some old files"""
        if os.path.exists(self.install_prefix):
            files = os.listdir(self.install_prefix)
            if files:
                RuntimeError(
                    "Install prefix path '{}' contains files {}".format(
                        self.install_prefix, files
                    )
                )

    def tearDown(self):
        """Remove created files"""
        silent_rmtree(self.install_prefix)

    def test_github_install(self):
        """Test installing extension from github"""

        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://github.com/wenzeslaus/r.example.plus",
            prefix=self.install_prefix,
        )

        # Modules with non-standard branch would be good for testing...
        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://github.com/wenzeslaus/r.example.plus",
            prefix=self.install_prefix,
        )

        for file in self.files:
            self.assertFileExists(file)

    def test_gitlab_install(self):
        """Test installing extension from gitlab"""
        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://gitlab.com/vpetras/r.example.plus",
            prefix=self.install_prefix,
        )

        for file in self.files:
            self.assertFileExists(file)

    def test_bitbucket_install(self):
        """Test installing extension from bitbucket"""
        d_rast_files = [
            os.path.join(self.install_prefix, "scripts", "r.sim.stats"),
            os.path.join(self.install_prefix, "docs", "html", "r.sim.stats.html"),
        ]
        self.assertModule(
            "g.extension",
            extension="r.sim.stats",
            url="https://bitbucket.org/lrntct/r.sim.stats",
            prefix=self.install_prefix,
        )

        for file in d_rast_files:
            self.assertFileExists(file)

    def test_github_install_official(self):
        """Test installing C-extension from official addons repository"""
        r_gdd_files = [
            os.path.join(self.install_prefix, "bin", "r.gdd"),
            os.path.join(self.install_prefix, "docs", "html", "r.gdd.html"),
        ]
        self.assertModule("g.extension", extension="r.gdd", prefix=self.install_prefix)

        for file in r_gdd_files:
            self.assertFileExists(file)

    def test_github_install_official_multimodule(self):
        """Test installing multi-module extension from official addons repository"""
        i_sentinel_files = [
            os.path.join(
                self.install_prefix, "scripts", "i.sentinel.parallel.download"
            ),
            os.path.join(
                self.install_prefix, "docs", "html", "i.sentinel.parallel.download.html"
            ),
            os.path.join(self.install_prefix, "scripts", "i.sentinel.import"),
            os.path.join(self.install_prefix, "docs", "html", "i.sentinel.import.html"),
        ]
        self.assertModule(
            "g.extension", extension="i.sentinel", prefix=self.install_prefix
        )

        for file in i_sentinel_files:
            self.assertFileExists(file)
            if not file.endswith("html"):
                print(file)
                self.assertModule(file, **{"help": True})

    def test_windows_install(self):
        """Test function for installing extension on MS Windows"""

        if not os.path.exists(self.install_prefix):
            os.mkdir(self.install_prefix)

        v_in_gbif_files = [
            os.path.join(self.install_prefix, "v.in.gbif", "bin", "v.in.gbif.bat"),
            os.path.join(self.install_prefix, "v.in.gbif", "scripts", "v.in.gbif.py"),
            os.path.join(
                self.install_prefix, "v.in.gbif", "docs", "html", "v.in.gbif.html"
            ),
        ]
        g_extension_path = find_executable("g.extension")
        g_extension = SourceFileLoader("g.extension", g_extension_path).load_module()
        g_extension.options = {
            "extension": "v.in.gbif",
            "operation": "add",
            "prefix": self.install_prefix,
        }
        g_extension.build_platform = "x86_64"
        g_extension.version = "785"
        g_extension.TMPDIR = self.install_prefix
        g_extension.install_extension_win("v.in.gbif")

        for file in v_in_gbif_files:
            self.assertFileExists(file)


if __name__ == "__main__":
    test()
