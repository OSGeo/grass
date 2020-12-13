# -*- coding: utf-8 -*-
import os
import sys
import platform

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

import grass.script as gs
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
        self.assertRaises(ValueError, utils.append_random, base_name, total_length=size)


class TestLegalizeVectorName(TestCase):
    """Tests legalize_vector_name() function"""

    # Names for general tests (input, output)
    # Not ideal as the only tests don't show which one failed for errors which
    # are not assert failures.
    # Good for adding new combinations.
    names = [
        ("a", "a"),
        ("__abc__", "x__abc__"),
        ("125", "x125"),
        ("1a25g78g", "x1a25g78g"),
        ("_X1", "x_X1"),
        ("øaøbøcø", "x_a_b_c_"),
    ]

    def works_for_vector_table_column(self, name):
        """Try to create vector, with attribute table and a column with name

        Returns false when that fails. Does not report further errors.
        Use together with other tests.
        """
        try:
            gs.run_command("v.edit", map=name, tool="create")
            gs.run_command("v.db.addtable", map=name)
            gs.run_command("v.db.addcolumn", map=name, columns=name)
            works = True
        except gs.CalledModuleError:
            works = False
        finally:
            gs.run_command("g.remove", name=name, type="vector", flags="f")
        return works

    def test_is_legal_name(self):
        """Check that it is a G_legal_name()"""
        for name, reference in self.names:
            legalized = utils.legalize_vector_name(name)
            self.assertTrue(legal_name(legalized))
            self.assertEqual(legalized, reference)

    def test_is_working_in_vector_table_column(self):
        """Check that a vector and column can be created

        This indirectly tests that it is Vect_legal_name().
        """
        for name, reference in self.names:
            legalized = utils.legalize_vector_name(name)
            self.assertTrue(self.works_for_vector_table_column(legalized))
            self.assertEqual(legalized, reference)

    def test_no_change(self):
        """There should be no change if the name is valid already"""
        name = "perfectly_valid_name"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, name)
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_has_dashes(self):
        """Check behavior with dash (a typical and important case)"""
        name = "abc-def-1"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "abc_def_1")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_has_spaces(self):
        """Check behavior with a space"""
        name = "abc def 1"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "abc_def_1")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_has_at_sign(self):
        """Check behavior with @

        This can happen, e.g., when a full map name is used, so testing
        explicitly.
        """
        name = "abc@def"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "abc_def")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_has_dollar(self):
        """Check with one invalid character"""
        name = "abc_$def"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "abc__def")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_with_ascii_art(self):
        """Check with a lot of invalid characters"""
        name = "abc_>>>def<<<_!"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "abc____def_____")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_starts_with_digit(self):
        """Check string starting with digit"""
        name = "123456"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "x123456")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_starts_with_underscore(self):
        """Check string starting with underscore"""
        name = "_123456"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "x_123456")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_has_unicode(self):
        """Check string with unicode"""
        name = "abøc"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "ab_c")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_starts_with_unicode(self):
        """Check string starting with unicode"""
        name = "øabc"
        legalized = utils.legalize_vector_name(name)
        self.assertEqual(legalized, "x_abc")
        self.assertTrue(legal_name(legalized))
        self.assertTrue(self.works_for_vector_table_column(legalized))

    def test_diff_only_first_character(self):
        """Check two string with only the first character being different"""
        name1 = "1800"
        name2 = "2800"
        legalized1 = utils.legalize_vector_name(name1)
        legalized2 = utils.legalize_vector_name(name2)
        self.assertNotEqual(legalized1, legalized2)

    def test_custom_prefix(self):
        """Check providing custom prefix"""
        name = "1800"
        prefix = "prefix_a1_"
        legalized = utils.legalize_vector_name(name, fallback_prefix=prefix)
        self.assertEqual(len(prefix) + len(name), len(legalized))
        self.assertIn(prefix, legalized)

    def test_no_prefix(self):
        """Check providing custom prefix"""
        name = "1800"
        legalized = utils.legalize_vector_name(name, fallback_prefix="")
        self.assertEqual(len(name), len(legalized))


if __name__ == "__main__":
    test()
