# -*- coding: utf-8 -*-

import copy
import subprocess

from grass.pygrass.modules import Module

import gunittest
from gunittest.gmodules import CalledModuleError


class TestModuleAssertions(gunittest.TestCase):
    # pylint: disable=R0904

    def setUp(self):
        self.rinfo = Module('r.info', map='elevation', flags='g',
                       stdout_=subprocess.PIPE, run_=False)
        self.rinfo_wrong = copy.deepcopy(self.rinfo)
        self.wrong_map = 'does_not_exists'
        self.rinfo_wrong.inputs['map'].value = self.wrong_map

    def test_runModule(self):
        self.runModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs['stdout'].value)
        self.assertRaises(CalledModuleError, self.runModule, self.rinfo_wrong)

    def test_assertModule(self):
        self.assertModule(self.rinfo)
        self.assertTrue(self.rinfo.outputs['stdout'].value)
        self.assertRaises(self.failureException, self.assertModule, self.rinfo_wrong)

    def test_assertModuleFail(self):
        self.assertModuleFail(self.rinfo_wrong)
        stderr = self.rinfo_wrong.outputs['stderr'].value
        self.assertTrue(stderr)
        self.assertIn(self.wrong_map, stderr)
        self.assertRaises(self.failureException, self.assertModuleFail, self.rinfo)


if __name__ == '__main__':
    gunittest.test()
