"""
Created on Tue Jun 24 09:43:53 2014

@author: pietro
"""
import sys
from fnmatch import fnmatch
from io import BytesIO as StringIO

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script.core import get_commands
from grass.exceptions import ParameterError
from grass.pygrass.modules.interface import Module


SKIP = [
    "g.parser",
]


# taken from six
def with_metaclass(meta, *bases):
    """Create a base class with a metaclass."""

    # This requires a bit of explanation: the basic idea is to make a dummy
    # metaclass for one level of class instantiation that replaces itself with
    # the actual metaclass.
    class metaclass(meta):
        def __new__(cls, name, this_bases, d):
            return meta(name, bases, d)

    return type.__new__(metaclass, "temporary_class", (), {})


class ModulesMeta(type):
    def __new__(mcs, name, bases, dict):
        def gen_test(cmd):
            def test(self):
                Module(cmd)

            return test

        cmds = [
            c
            for c in sorted(list(get_commands()[0]))
            if c not in SKIP and not fnmatch(c, "g.gui.*")
        ]
        for cmd in cmds:
            test_name = "test__%s" % cmd.replace(".", "_")
            dict[test_name] = gen_test(cmd)
        return type.__new__(mcs, name, bases, dict)


class TestModules(with_metaclass(ModulesMeta, TestCase)):
    pass


class TestModulesPickability(TestCase):
    def test_rsun(self):
        """Test if a Module instance is pickable"""
        import pickle

        out = StringIO()
        pickle.dump(Module("r.sun"), out)
        out.close()


class TestModulesJsonDictExport(TestCase):
    def test_rinfo_simple(self):
        """Test if a Module can be exported to json dict"""

        Module("r.info", map="elevation", run_=False).get_json_dict()

    def test_rinfo_ov(self):
        """Test if a Module can be exported to json dict with overwrite
        and verbose flags"""

        Module(
            "r.info", overwrite=True, verbose=True, map="elevation", run_=False
        ).get_json_dict()

    def test_rinfo_ov_export(self):
        """Test if a Module can be exported to json dict with overwrite
        and verbose flags and results exported to CSV"""

        Module(
            "r.info",
            overwrite=True,
            verbose=True,
            map="elevation",
            flags="g",
            run_=False,
        ).get_json_dict(export="CSV")

    def test_rslopeaspect_ov_export(self):
        """Test if a Module can be exported to json dict with overwrite
        and verbose flags and results exported to CSV"""

        Module(
            "r.slope.aspect",
            elevation="elevation",
            slope="slope",
            aspect="aspect",
            overwrite=True,
            verbose=True,
            map="elevation",
            flags="g",
            run_=False,
        ).get_json_dict(export="GTiff")


class TestModulesCheck(TestCase):
    def test_flags_with_suppress_required(self):
        """Test if flags with suppress required are handle correctly"""
        gextension = Module("g.extension")
        # check if raise an error if required parameter are missing
        with self.assertRaises(ParameterError):
            gextension.check()

        # check if the flag suppress the required parameters
        gextension.flags.a = True
        self.assertIsNone(gextension.check())


if __name__ == "__main__":
    test()
