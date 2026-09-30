"""
SPDX-FileCopyrightText: 2026 GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later

Unit tests for grass.pygrass.vector.basic
"""

from unittest.mock import patch

from grass.exceptions import GrassError
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.pygrass.vector.basic import Ilist


class IlistTestCase(TestCase):
    def test_init_empty(self):
        """Test empty Ilist initialization"""
        ilist = Ilist()
        self.assertEqual(len(ilist), 0)
        self.assertEqual(list(ilist), [])

    def test_init_with_list(self):
        """Test Ilist initialization with integer list"""
        ilist = Ilist([1, 2, 3])
        self.assertEqual(len(ilist), 3)
        self.assertEqual(list(ilist), [1, 2, 3])

    def test_getitem(self):
        """Test __getitem__ with indexing and slicing"""
        ilist = Ilist([10, 20, 30, 40])
        self.assertEqual(ilist[0], 10)
        self.assertEqual(ilist[1], 20)
        self.assertEqual(ilist[-1], 40)
        self.assertEqual(ilist[1:3], [20, 30])
        with self.assertRaises(IndexError):
            _ = ilist[10]

    def test_setitem(self):
        """Test __setitem__"""
        ilist = Ilist([1, 2, 3])
        ilist[1] = 20
        self.assertEqual(list(ilist), [1, 20, 3])
        with self.assertRaises(ValueError):
            ilist[0] = 3

    def test_contains(self):
        """Test contains and in operator"""
        ilist = Ilist([1, 2, 3])
        self.assertTrue(1 in ilist)
        self.assertFalse(4 in ilist)
        self.assertTrue(ilist.contains(2))
        self.assertFalse(ilist.contains(5))

    def test_append(self):
        """Test appending an integer"""
        ilist = Ilist([1, 2])
        ilist.append(3)
        self.assertEqual(list(ilist), [1, 2, 3])
        # Appending duplicate should not duplicate
        ilist.append(2)
        self.assertEqual(list(ilist), [1, 2, 3])

    def test_append_error(self):
        """Test append raises GrassError when Vect_list_append fails"""
        ilist = Ilist()
        with (
            patch(
                "grass.pygrass.vector.basic.libvect.Vect_list_append",
                return_value=1,
            ),
            self.assertRaises(GrassError),
        ):
            ilist.append(10)

    def test_extend_with_python_list(self):
        """Test extending Ilist with Python list"""
        ilist = Ilist([1, 2])
        ilist.extend([3, 4])
        self.assertEqual(list(ilist), [1, 2, 3, 4])

    def test_extend_with_ilist(self):
        """Test extending Ilist with another Ilist instance"""
        ilist_a = Ilist([1, 2, 3])
        ilist_b = Ilist([4, 5, 6])
        ilist_a.extend(ilist_b)
        self.assertEqual(list(ilist_a), [1, 2, 3, 4, 5, 6])

    def test_remove_int(self):
        """Test removing an integer"""
        ilist = Ilist([1, 2, 3, 4])
        ilist.remove(2)
        self.assertEqual(list(ilist), [1, 3, 4])

    def test_remove_ilist(self):
        """Test removing items from another Ilist instance"""
        ilist_a = Ilist([1, 2, 3, 4, 5])
        ilist_b = Ilist([2, 4])
        ilist_a.remove(ilist_b)
        self.assertEqual(list(ilist_a), [1, 3, 5])

    def test_remove_iterable(self):
        """Test removing items with an iterable"""
        ilist = Ilist([1, 2, 3, 4, 5])
        ilist.remove([1, 3])
        self.assertEqual(list(ilist), [2, 4, 5])

    def test_remove_invalid(self):
        """Test removing unsupported value type raises ValueError"""
        ilist = Ilist([1, 2])
        with self.assertRaises(ValueError):
            ilist.remove("invalid")

    def test_reset(self):
        """Test resetting Ilist"""
        ilist = Ilist([1, 2, 3])
        ilist.reset()
        self.assertEqual(len(ilist), 0)
        self.assertEqual(list(ilist), [])


if __name__ == "__main__":
    test()
