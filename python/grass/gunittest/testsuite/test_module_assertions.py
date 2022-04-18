# use %%python here to run this code in jupyterlab binder
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
        """Run PyGRASS module. Runs the module and raises an exception if the module ends with non-zero return code. 
        Usually, this is the same as testing the return code and raising exception but by using this method, you give testing framework more control over the execution, error handling and storing of output.
        In terms of testing framework, this function causes a common error, not a test failure."""
        self.runModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(CalledModuleError, self.runModule, self.rinfo_wrong)

    def test_assertModule(self):
        """Correct and incorrect Module used in assertModule()"""
        """Run PyGRASS module in controlled way and assert non-zero return code.
        You should use this method to invoke module you are testing. 
        By using this method, you give testing framework more control over the execution, error handling and storing of output.
        It will not print module stdout and stderr, instead it will always store them for further examination. Streams are stored separately.
        This method is not suitable for testing error states of the module. """
        self.assertModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(self.failureException, self.assertModule, self.rinfo_wrong)

    def test_assertModuleFail(self):
        """Correct and incorrect Module used in assertModuleFail()"""
        """Test that module fails with a non-zero return code.
        Works like assertModule() but expects module to fail."""
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
        """Run PyGRASS module. Runs the module and raises an exception if the module ends with non-zero return code. 
        Usually, this is the same as testing the return code and raising exception but by using this method, you give testing framework more control over the execution, error handling and storing of output.
        In terms of testing framework, this function causes a common error, not a test failure."""
        self.runModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(CalledModuleError, self.runModule, self.rinfo_wrong)

    def test_assertModule(self):
        """Correct and incorrect SimpleModule used in assertModule()"""
        """Run PyGRASS module in controlled way and assert non-zero return code.
        You should use this method to invoke module you are testing. 
        By using this method, you give testing framework more control over the execution, error handling and storing of output.
        It will not print module stdout and stderr, instead it will always store them for further examination. Streams are stored separately.
        This method is not suitable for testing error states of the module. """
        self.assertModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs["stdout"].value)
        self.assertRaises(self.failureException, self.assertModule, self.rinfo_wrong)

    def test_assertModuleFail(self):
        """Correct and incorrect SimpleModule used in assertModuleFail()"""
        """Test that module fails with a non-zero return code.
        Works like assertModule() but expects module to fail."""
        self.assertModuleFail(self.rinfo_wrong)
        stderr = self.rinfo_wrong.outputs["stderr"].value
        self.assertTrue(stderr)
        self.assertIn(self.wrong_map, stderr)
        self.assertRaises(self.failureException, self.assertModuleFail, self.rinfo)


if __name__ == "__main__":
    test()
