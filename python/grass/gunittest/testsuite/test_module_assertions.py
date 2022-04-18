import copy
import subprocess

from grass.pygrass.modules import Module
from grass.gunittest.gmodules import SimpleModule

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import CalledModuleError


class TestModuleAssertions(TestCase):
    """Test assertions using PyGRASS Module"""

    # pylint: disable=R0904

    def setUp(self):
        """Create two Module instances one correct and one with wrong map"""
        self.rinfo = Module(
            "r.info",
            map="elevation",
            flags="g",
            stdout_=subprocess.PIPE,
            run_=False,
            finish_=True,
        )
        self.rinfo_wrong = copy.deepcopy(self.rinfo)
        self.wrong_map = "does_not_exists"
        self.rinfo_wrong.inputs["map"].value = self.wrong_map

    def test_runModule(self):
        """Correct and incorrect Module used in runModule()"""
        self.runModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(CalledModuleError, self.runModule, self.rinfo_wrong)

    def test_assertModule(self):
        """Correct and incorrect Module used in assertModule()"""
        self.assertModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(self.failureException, self.assertModule, self.rinfo_wrong)

    def test_assertModuleFail(self):
        """Correct and incorrect Module used in assertModuleFail()"""
        self.assertModuleFail(self.rinfo_wrong)
        stderr = self.rinfo_wrong.outputs["stderr"].value
        self.assertTrue(stderr)
        self.assertIn(self.wrong_map, stderr)
        self.assertRaises(self.failureException, self.assertModuleFail, self.rinfo)


class TestSimpleModuleAssertions(TestCase):
    """Test assertions using SimpleModule"""

    # pylint: disable=R0904

    def setUp(self):
        """Create two SimpleModule instances one correct and one with wrong map"""
        self.rinfo = SimpleModule("r.info", map="elevation", flags="g")
        self.rinfo_wrong = copy.deepcopy(self.rinfo)
        self.wrong_map = "does_not_exists"
        self.rinfo_wrong.inputs["map"].value = self.wrong_map

    def test_runModule(self):
        """Correct and incorrect SimpleModule used in runModule()"""
        self.runModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(CalledModuleError, self.runModule, self.rinfo_wrong)

    def test_assertModule(self):
        """Correct and incorrect SimpleModule used in assertModule()"""
        self.assertModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(self.failureException, self.assertModule, self.rinfo_wrong)

    def test_assertModuleFail(self):
        """Correct and incorrect SimpleModule used in assertModuleFail()"""
        self.assertModuleFail(self.rinfo_wrong)
        stderr = self.rinfo_wrong.outputs["stderr"].value
        self.assertTrue(stderr)
        self.assertIn(self.wrong_map, stderr)
        self.assertRaises(self.failureException, self.assertModuleFail, self.rinfo)


if __name__ == "__main__":
    test()
    
"""
north=228500
south=215000
east=645000
west=630000
nsres=10
ewres=10
rows=1350
cols=1500
cells=2025000
datatype=FCELL
ncats=255
ERROR: Raster map <does_not_exists> not found
ERROR: Raster map <does_not_exists> not found
north=228500
south=215000
east=645000
west=630000
nsres=10
ewres=10
rows=1350
cols=1500
cells=2025000
datatype=FCELL
ncats=255
north=228500
south=215000
east=645000
west=630000
nsres=10
ewres=10
rows=1350
cols=1500
cells=2025000
datatype=FCELL
ncats=255
ERROR: Raster map <does_not_exists> not found
ERROR: Raster map <does_not_exists> not found
north=228500
south=215000
east=645000
west=630000
nsres=10
ewres=10
rows=1350
cols=1500
cells=2025000
datatype=FCELL
ncats=255
......
----------------------------------------------------------------------
Ran 6 tests in 1.076s
OK
"""
