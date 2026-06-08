"""
Test t.rast.import

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author: lucadelu
"""

import os
import pathlib
import shutil

import grass.script as gs
from grass.gunittest.case import TestCase


class TestRasterImport(TestCase):
    input_ = os.path.join("data", "precip_2000.tar.bzip2")
    new_project_name = "test_project_import"

    @classmethod
    def tearDownClass(cls):
        """Remove the imported data"""
        cls.runModule("t.remove", flags="df", inputs="A")

    def test_import(self):
        self.assertModule(
            "t.rast.import",
            flags="o",
            input=self.input_,
            output="A",
            basename="a",
            overwrite=True,
        )
        tinfo = """start_time='2000-01-01 00:00:00'
                   end_time='2001-01-01 00:00:00'
                   granularity='1 month'
                   map_time=interval
                   north=320000.0
                   south=10000.0
                   east=935000.0
                   west=120000.0"""

        self.assertModuleKeyValue(
            module="t.info", input="A", flags="g", reference=tinfo, precision=2, sep="="
        )

    def test_import_new_project(self):
        """Test the project option."""
        path = os.path.join(gs.gisenv()["GISDBASE"], self.new_project_name)

        shutil.rmtree(path, ignore_errors=True)
        self.addCleanup(shutil.rmtree, path, ignore_errors=True)

        self.assertModule(
            "t.rast.import",
            flags="o",
            input=self.input_,
            output="B",
            basename="b",
            project=self.new_project_name,
            overwrite=True,
        )

        self.assertTrue(pathlib.Path(path).exists())


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
