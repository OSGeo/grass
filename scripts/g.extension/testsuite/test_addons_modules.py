"""
TEST:      test_addons_modules.py

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test for g.extension individual modules/extensions handling

COPYRIGHT: (C) 2015 Vaclav Petras, and by the GRASS Development Team

           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule
from grass.gunittest.utils import silent_rmtree
from grass.script.utils import decode

import os


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
""".replace('\n', os.linesep)


class TestModulesMetadata(TestCase):

    url = 'file://' + os.path.abspath('data')

    def test_listing(self):
        """List individual extensions/modules/addons"""
        module = SimpleModule('g.extension', flags='l', url=self.url)
        self.assertModule(module)
        stdout = decode(module.outputs.stdout)
        self.assertMultiLineEqual(stdout, MODULES_OUTPUT)


class TestModulesFromDifferentSources(TestCase):

    url = 'file://' + os.path.abspath('data/sample_modules')
    path = os.path.join('data', 'sample_modules')
    install_prefix = 'gextension_test_install_path'
    # TODO: this is wrong for MS Win
    files = [
        os.path.join(install_prefix, 'scripts', 'r.plus.example'),
        os.path.join(install_prefix, 'docs', 'html', 'r.plus.example.html'),
    ]
    # to create archives from the source, the following was used:
    # zip r.plus.example.zip r.plus.example/*
    # tar czvf r.plus.example.tar.gz r.plus.example
    # cd r.plus.example/
    # tar czvf ../r.plus.example_sep.tar.gz *

    def setUp(self):
        """Make sure we are not dealing with some old files"""
        if os.path.exists(self.install_prefix):
            files = os.listdir(self.install_prefix)
            if files:
                RuntimeError("Install prefix path '{}' contains files {}"
                             .format(self.install_prefix, files))

    def tearDown(self):
        """Remove created files"""
        silent_rmtree(self.install_prefix)

    def test_directory_install(self):
        """Test installing extension from directory"""
        self.assertModule('g.extension', extension='r.plus.example',
                          url=os.path.join(self.path, 'r.plus.example'),
                          prefix=self.install_prefix)
        # TODO: this is wrong for MS Win
        for file in self.files:
            self.assertFileExists(file)

    def test_targz_install(self):
        """Test installing extension from local .tar.gz"""
        self.assertModule('g.extension', extension='r.plus.example',
                          url=os.path.join(self.path,
                                           'r.plus.example.tar.gz'),
                          prefix=self.install_prefix)
        for file in self.files:
            self.assertFileExists(file)

    def test_remote_targz_without_dir_install(self):
        """Test installing extension from (remote) .tar.gz without main dir"""
        self.assertModule('g.extension', extension='r.plus.example',
                          url=self.url + '/' + 'r.plus.example_sep.tar.gz',
                          prefix=self.install_prefix, verbose=True)
        for file in self.files:
            self.assertFileExists(file)

    def test_remote_zip_install(self):
        """Test installing extension from .zip specified by URL (local)"""
        self.assertModule('g.extension', extension='r.plus.example',
                          url=self.url + '/' + 'r.plus.example.zip',
                          prefix=self.install_prefix)
        for file in self.files:
            self.assertFileExists(os.path.join(file))


if __name__ == '__main__':
    test()
