"""
TEST:      test_addons_modules.py

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test for g.extension individual modules/extensions handling

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import xml.etree.ElementTree as ET

from pathlib import Path

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.utils import silent_rmtree, xfail_windows
from grass.script.utils import decode


MODULES_OUTPUT = """\
d.frame
d.mon2
g.copyall
g.isis3mt
g.proj.all
r.gdd
r.geomorphon
r.le.patch
r.le.pixel
r.traveltime
r.univar2
v.civil
v.class.ml
v.class.mlpy
v.colors2
v.delaunay3d
v.ellipse
v.in.proj
v.in.redwg
v.neighborhoodmatrix
v.transects
wx.metadata
""".replace("\n", os.linesep)


class TestModulesMetadata(TestCase):
    url = "file://" + os.path.abspath("data")

    @xfail_windows
    def test_listing(self):
        """List individual extensions/modules/addons"""
        module = SimpleModule("g.extension", flags="l", url=self.url)
        self.assertModule(module)
        stdout = decode(module.outputs.stdout)
        self.assertMultiLineEqual(stdout, MODULES_OUTPUT)


class TestModulesFromDifferentSources(TestCase):
    url = "file://" + os.path.abspath("data/sample_modules")
    path = os.path.join("data", "sample_modules")
    install_prefix = "gextension_test_install_path"
    # TODO: this is wrong for MS Win
    files = [
        os.path.join(install_prefix, "scripts", "r.plus.example"),
        os.path.join(install_prefix, "docs", "html", "r.plus.example.html"),
        os.path.join(install_prefix, "docs", "mkdocs", "source", "r.plus.example.md"),
    ]
    # to create archives from the source, the following was used:
    # zip r.plus.example.zip r.plus.example/*
    # tar czvf r.plus.example.tar.gz r.plus.example
    # cd r.plus.example/
    # tar czvf ../r.plus.example_sep.tar.gz *

    def setUp(self):
        """Make sure we are not dealing with some old files"""
        if Path(self.install_prefix).exists():
            files = [p.name for p in Path(self.install_prefix).iterdir()]
            if files:
                msg = "Install prefix path '{}' contains files {}".format(
                    self.install_prefix, files
                )
                raise RuntimeError(msg)

    def tearDown(self):
        """Remove created files"""
        silent_rmtree(self.install_prefix)

    @xfail_windows
    def test_directory_install(self):
        """Test installing extension from directory"""
        self.assertModule(
            "g.extension",
            extension="r.plus.example",
            url=os.path.join(self.path, "r.plus.example"),
            prefix=self.install_prefix,
        )
        # TODO: this is wrong for MS Win
        for file in self.files:
            self.assertFileExists(file)

    @xfail_windows
    def test_targz_install(self):
        """Test installing extension from local .tar.gz"""
        self.assertModule(
            "g.extension",
            extension="r.plus.example",
            url=os.path.join(self.path, "r.plus.example.tar.gz"),
            prefix=self.install_prefix,
        )
        for file in self.files:
            self.assertFileExists(file)

    @xfail_windows
    def test_remote_targz_without_dir_install(self):
        """Test installing extension from (remote) .tar.gz without main dir"""
        self.assertModule(
            "g.extension",
            extension="r.plus.example",
            url=self.url + "/" + "r.plus.example_sep.tar.gz",
            prefix=self.install_prefix,
            verbose=True,
        )
        for file in self.files:
            self.assertFileExists(file)

    @xfail_windows
    def test_remote_zip_install(self):
        """Test installing extension from .zip specified by URL (local)"""
        self.assertModule(
            "g.extension",
            extension="r.plus.example",
            url=self.url + "/" + "r.plus.example.zip",
            prefix=self.install_prefix,
        )
        for file in self.files:
            self.assertFileExists(os.path.join(file))


class TestModulesWxGuiToolsTreeAddonsNodeAddonsRegistration(TestCase):
    # MS Windows install function requires absolute paths
    install_prefix = Path("gextension_test_install_path").absolute()
    registered_modules_xml_file = install_prefix / "modules.xml"

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

    def parse_wxgui_tools_tree_addons_modules_xml_file(self):
        """Parse wxGUI Tools tree Addons modules.xml file

        :return list registered_modules: list of registered addons names
        """
        tree = ET.parse(self.registered_modules_xml_file)
        root = tree.getroot()
        registered_modules = []
        for submodule in root.findall("task"):
            registered_modules.append(submodule.attrib["name"])
        return registered_modules

    def test_simple_addon_registration(self):
        """Testing if simple addon is registered in the wxGUI Tools tree Addons node
        if it is used addons custom base dir
        """
        extension = "db.join"
        self.assertModule(
            "g.extension",
            extension=extension,
            prefix=str(self.install_prefix),
        )
        self.assertIn(
            extension,
            self.parse_wxgui_tools_tree_addons_modules_xml_file(),
        )

    def test_multi_addons_registration(self):
        """Testing if multi addona are registered in the wxGUI Tools tree Addons node
        if it is used addons custom base dir
        """
        extension = "wx.metadata"
        extension_modules = (
            "db.csw.admin",
            "db.csw.harvest",
            "db.csw.run",
            "g.gui.cswbrowser",
            "g.gui.metadata",
            "m.csw.update",
            "r.info.iso",
            "t.info.iso",
            "v.info.iso",
        )
        self.assertModule(
            "g.extension",
            extension=extension,
            prefix=str(self.install_prefix),
        )
        registered_extension_modules = (
            self.parse_wxgui_tools_tree_addons_modules_xml_file()
        )
        for addon in extension_modules:
            self.assertIn(
                addon,
                registered_extension_modules,
            )


if __name__ == "__main__":
    test()
