"""
TEST:      test_addons_download.py

AUTHOR(S): Stefan Blumentrath <stefan dot blumentrath at nina dot no)

PURPOSE:   Test for g.extension individual modules/extensions download

COPYRIGHT: (C) 2022 Stefan Blumentrath and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import re
import sys
import unittest

from pathlib import Path
from urllib import request as urlrequest

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.main import test
from grass.gunittest.utils import silent_rmtree

ms_windows = sys.platform == "win32" or sys.platform == "cygwin"


class TestModuleDownloadFromDifferentSources(TestCase):
    """Tests if addons are downloaded and installed successfully
    by checking that respective files are present in the prefix directory
    and that the addon starts (help is printed)
    Based on test_addons_modules.py bym Vaclav Petras <wenzeslaus gmail com>
    """

    # MS Windows install function requires absolute paths
    install_prefix = Path("gextension_test_install_path").absolute()

    files = [
        install_prefix / "scripts" / "r.example.plus",
        install_prefix / "docs" / "html" / "r.example.plus.html",
    ]

    request_headers = {
        "User-Agent": "Mozilla/5.0",
    }

    def setUp(self):
        """Make sure we are not dealing with some old files"""
        if self.install_prefix.exists():
            files = [path.name for path in self.install_prefix.iterdir()]
            if files:
                msg = f"Install prefix path '{self.install_prefix}' \
                    contains files {','.join(files)}"
                raise RuntimeError(msg)

    def tearDown(self):
        """Remove created files"""
        silent_rmtree(str(self.install_prefix))

    @unittest.skipIf(ms_windows, "currently not supported on MS Windows")
    def test_github_install(self):
        """Test installing extension from github"""

        # Modules with non-standard branch would be good for testing...
        self.assertModule(
            "g.extension",
            extension="r.example.plus",
            url="https://github.com/wenzeslaus/r.example.plus",
            prefix=str(self.install_prefix),
        )

        for file in self.files:
            self.assertFileExists(file)
            if file.suffix != ".html":
                self.assertModule(str(file), help=True)

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
            if file.suffix != ".html":
                self.assertModule(str(file), help=True)

    @unittest.skipIf(ms_windows, "currently not supported on MS Windows")
    def test_bitbucket_install(self):
        """Test installing extension from bitbucket"""
        files = [
            self.install_prefix / "scripts" / "r.sim.stats",
            self.install_prefix / "docs" / "html" / "r.sim.stats.html",
        ]
        self.assertModule(
            "g.extension",
            extension="r.sim.stats",
            url="https://bitbucket.org/lrntct/r.sim.stats",
            prefix=str(self.install_prefix),
        )

        for file in files:
            self.assertFileExists(file)
            if file.suffix != ".html":
                self.assertModule(str(file), help=True)

    def test_github_install_official(self):
        """Test installing C-extension from official addons repository"""
        files = [
            self.install_prefix / "docs" / "html" / "r.gdd.html",
        ]
        if ms_windows:
            files.append(self.install_prefix / "bin" / "r.gdd.exe")
        else:
            files.append(self.install_prefix / "bin" / "r.gdd")

        self.assertModule(
            "g.extension", extension="r.gdd", prefix=str(self.install_prefix)
        )

        for file in files:
            self.assertFileExists(file)
            if file.suffix != ".html":
                self.assertModule(str(file), help=True)

    def test_github_install_official_multimodule(self):
        """Test installing multi-module extension from official addons repository"""
        files = [
            self.install_prefix / "docs" / "html" / "i.sentinel.parallel.download.html",
            self.install_prefix / "docs" / "html" / "i.sentinel.import.html",
        ]
        if ms_windows:
            files.extend(
                [
                    self.install_prefix / "scripts" / "i.sentinel.parallel.download.py",
                    self.install_prefix / "scripts" / "i.sentinel.import.py",
                    self.install_prefix / "bin" / "i.sentinel.parallel.download.bat",
                    self.install_prefix / "bin" / "i.sentinel.import.bat",
                ]
            )
        else:
            files.extend(
                [
                    self.install_prefix / "scripts" / "i.sentinel.parallel.download",
                    self.install_prefix / "scripts" / "i.sentinel.import",
                ]
            )

        self.assertModule(
            "g.extension", extension="i.sentinel", prefix=str(self.install_prefix)
        )

        for file in files:
            self.assertFileExists(file)
            if file.suffix not in {".html", ".py"}:
                self.assertModule(str(file), help=True)

    def test_github_install_official_non_exists_module(self):
        """Test installing non exists extension from official addons repository"""
        extension = "non_exists_extension"
        gextension = SimpleModule("g.extension", extension=extension)
        self.assertModuleFail(gextension)
        self.assertTrue(gextension.outputs.stderr)
        self.assertIn(extension, gextension.outputs.stderr)

    def test_github_download_official_module_src_code_only(self):
        """Test download extension source code only from official addons
        repository and check extension temporary directory path"""
        gextension = SimpleModule(
            "g.extension",
            extension="db.join",
            flags="d",
        )
        self.assertModule(gextension)
        self.assertTrue(gextension.outputs.stderr)
        ext_path_str = re.search(
            rf"^{_('Path to the source code:')}(\n|\n\n)(.+?)\n",
            gextension.outputs.stderr,
            re.MULTILINE,
        )
        self.assertTrue(ext_path_str)
        ext_path = Path(ext_path_str.group(2))
        self.assertTrue(ext_path.exists())
        self.assertIn(ext_path / "Makefile", list(ext_path.iterdir()))

    def test_github_official_module_man_page_src_code_links_exists(self):
        """Test if the installed extension HTML manual page from the
        official GitHub repository contains the source code link and
        the source code history link and if they exists
        """
        extension = "db.join"
        self.assertModule(
            "g.extension",
            extension=extension,
            prefix=str(self.install_prefix),
        )
        html_man_page = self.install_prefix / "docs" / "html" / "db.join.html"
        self.assertFileExists(str(html_man_page))
        content = Path(html_man_page).read_text()
        for link_name in [f"{extension} source code", "history"]:
            url = re.search(rf"<a href=\"(.*)\">{link_name}</a>", content).group(1)
            self.assertTrue(url)
            try:
                request = urlrequest.Request(url, headers=self.request_headers)
                response = urlrequest.urlopen(request).code
            except urlrequest.HTTPError as e:
                response = e.code
            except urlrequest.URLError as e:
                response = e.args
            self.assertEqual(response, 200)

    def test_github_install_official_multimodule_and_check_metadata(self):
        """Test installing multi-module extension from official addons
        repository without printing warning no metadata available message
        for module wich install HTML page file only"""
        extension = "i.sentinel"
        gextension = SimpleModule(
            "g.extension",
            extension=extension,
            prefix=str(self.install_prefix),
        )
        self.assertModule(gextension)
        self.assertTrue(gextension.outputs.stderr)
        self.assertNotIn(
            _("No metadata available for module '{}':").format(extension),
            gextension.outputs.stderr,
        )


if __name__ == "__main__":
    test()
