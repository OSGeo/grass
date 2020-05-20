# -*- coding: utf-8 -*-
import os
import sys
import platform

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.script import legal_name
from grass.script import utils


class TestUnique(TestCase):
    """Tests functions generating unique names and suffixes"""

    def test_append_node_pid(self):
        base_name = "tmp_abc"
        full_name = utils.append_node_pid(base_name)
        self.assertIn(base_name, full_name)
        self.assertGreater(len(full_name), len(base_name))
        self.assertIn(str(os.getpid()), full_name)
        self.assertIn(platform.node(), full_name)
        self.assertTrue(legal_name(full_name))
        # TODO: It should be also a valid vector name
        # but we don't have a function for that (same for all)
        full_name2 = utils.append_node_pid(base_name)
        self.assertEqual(
            full_name, full_name2, msg="There should be no randomness or change."
        )

    def test_append_uuid(self):
        base_name = "tmp_abc"
        full_name = utils.append_uuid(base_name)
        self.assertIn(base_name, full_name)
        self.assertGreater(len(full_name), len(base_name))
        self.assertTrue(legal_name(full_name))
        full_name2 = utils.append_uuid(base_name)
        # There is a low chance of collision.
        self.assertNotEqual(full_name, full_name2)

    def test_append_random_suffix(self):
        base_name = "tmp_abc"
        size = 10
        full_name = utils.append_random(base_name, suffix_length=size)
        self.assertIn(base_name, full_name)
        self.assertGreater(len(full_name), len(base_name))
        self.assertGreaterEqual(len(full_name), len(base_name) + size)
        self.assertTrue(legal_name(full_name))
        full_name2 = utils.append_random(base_name, suffix_length=size)
        # There is a low chance of collision.
        self.assertNotEqual(full_name, full_name2)

    def test_append_random_total(self):
        base_name = "tmp_abc"
        size = 10
        full_name = utils.append_random(base_name, total_length=size)
        self.assertIn(base_name, full_name)
        self.assertGreater(len(full_name), len(base_name))
        self.assertEqual(len(full_name), size)
        self.assertTrue(legal_name(full_name))
        full_name2 = utils.append_random(base_name, total_length=size)
        self.assertNotEqual(full_name, full_name2)

    def test_append_random_one_arg(self):
        base_name = "tmp_abc"
        size = 10
        full_name = utils.append_random(base_name, size)
        self.assertIn(base_name, full_name)
        self.assertGreater(len(full_name), len(base_name))
        self.assertGreaterEqual(len(full_name), len(base_name) + size)
        self.assertTrue(legal_name(full_name))
        full_name2 = utils.append_random(base_name, size)
        self.assertNotEqual(full_name, full_name2)

    def test_append_random_two_args(self):
        base_name = "tmp_abc"
        size = 10
        self.assertRaises(ValueError, utils.append_random, base_name, size, size)

    def test_append_random_total_name_too_long(self):
        base_name = "tmp_abc"
        size = 4
        self.assertRaises(
            ValueError, utils.append_random, base_name, total_length=size
        )


if __name__ == '__main__':
    test()
