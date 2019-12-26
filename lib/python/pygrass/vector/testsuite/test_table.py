# -*- coding: utf-8 -*-
"""
Created on Wed Jun 25 11:08:22 2014

@author: pietro
"""
import os
import sqlite3
import sys
import tempfile as tmp
from string import ascii_letters, digits
from random import choice
import numpy as np

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.vector.table import Table, get_path


if sys.version_info.major == 3:
    long = int

# dictionary that generate random data
COL2VALS = {'INT': lambda n:     np.random.randint(9, size=n),
            'INTEGER': lambda n: np.random.randint(9, size=n),
            'INTEGER PRIMARY KEY': lambda n: np.arange(1, n+1, dtype=long),
            'REAL': lambda n: np.random.rand(n),
            'TEXT': lambda n: np.array([randstr() for _ in range(n)])}


def randstr(prefix='', suffix='', size=6, chars=ascii_letters + digits):
    """Return a random string of characters.

    :param prefix: string prefix, default: ''
    :type prefix: str

    :param suffix: string suffix, default: ''
    :type suffix: str

    :param size: number of random characters
    :type size: int

    :param chars: string containing the characters that will be used
    :type chars: str

    :returns: string
    """
    return prefix + ''.join(choice(chars) for _ in range(size)) + suffix


def get_table_random_values(nrows, columns):
    """Generate a random recarray respecting the columns definition.

    :param nrows: number of rows of the generated array
    :type nrows: int

    :param columns: list of tuple containing column name and type.
    :type columns: list of tuple

    :returns: numpy recarray
    """
    vals, dtype = [], []
    for cname, ctype in columns:
        if ctype not in COL2VALS:
            raise TypeError("Unknown column type %s for: %s" % (ctype, cname))
        vals.append(COL2VALS[ctype](nrows))
        dtype.append((cname, vals[-1].dtype.str))
    return np.array([v for v in zip(*vals)], dtype=dtype)


class DBconnection(object):
    """Define a class to share common methods between TestCase."""
    path = os.path.join(tmp.gettempdir(), randstr(prefix='temp', suffix='.db'))
    connection = sqlite3.connect(get_path(path))
    for t in (np.int8, np.int16, np.int32, np.int64, np.uint8,
              np.uint16, np.uint32, np.uint64):
        sqlite3.register_adapter(t, int)
    columns = [('cat', 'INTEGER PRIMARY KEY'),
               ('cint', 'INT'),
               ('creal', 'REAL'),
               ('ctxt', 'TEXT')]

    def create_table_instance(self, **kw):
        """Return a Table class instance

        :param **kw: keyword arguments of Table class
                     without name and connection.
        :type **kw: key-word arguments

        :returns: Table instance
        """
        self.tname = randstr(prefix='temp')
        return Table(name=self.tname,
                     connection=self.connection, **kw)

    def create_empty_table(self, columns=None, **kw):
        """Create an empty table in the database and return Table class
        instance.

        :param columns: list of tuple containing the column names and types.
        :type columns: list of tuple

        :param **kw: keyword arguments of Table class
                     without name and connection.
        :type **kw: key-word arguments

        :returns: Table instance
        """
        columns = self.columns if columns is None else columns
        table = self.create_table_instance(**kw)
        table.create(columns)
        return table

    def create_not_empty_table(self, nrows=None, values=None,
                               columns=None, **kw):
        """Create a not empty table in the database and return Table class
        instance.

        :param nrows: number of rows.
        :type nrows: list of tuple

        :param values: list of tuple containing the values for each row.
        :type values: list of tuple

        :param columns: list of tuple containing the column names and types.
        :type columns: list of tuple

        :param **kw: keyword arguments of Table class
                     without name and connection.
        :type **kw: key-word arguments

        :returns: Table instance
        """
        if nrows is None and values is None:
            msg = "Both parameters ``nrows`` ``values`` are empty"
            raise RuntimeError(msg)
        columns = self.columns if columns is None else columns
        values = (get_table_random_values(nrows, columns) if values is None
                  else values)
        table = self.create_empty_table(columns=columns, **kw)
        table.insert(values, many=True)
        return table

    def setUp(self):
        """Create a not empty table instance"""
        self.table = self.create_not_empty_table(10)
        self.cols = self.table.columns

    def tearDown(self):
        """Remove the generated vector map, if exist"""
        self.table.drop(force=True)
        self.table = None
        self.cols = None


class ColumnsTestCase(DBconnection, TestCase):

    def test_check_insert_update_str(self):
        """Check insert_str and update_str attribute of Columns are correct"""
        insert = 'INSERT INTO %s VALUES (?,?,?,?)'
        self.assertEqual(self.cols.insert_str, insert % self.tname)
        update = 'UPDATE %s SET cint=?,creal=?,ctxt=? WHERE cat=?;'
        self.assertEqual(self.cols.update_str, update % self.tname)


class TableInsertTestCase(DBconnection, TestCase):

    def setUp(self):
        """Create a not empty table instance"""
        self.table = self.create_empty_table()
        self.cols = self.table.columns

    def tearDown(self):
        """Remove the generated vector map, if exist"""
        self.table.drop(force=True)
        self.table = None
        self.cols = None

    def test_insert(self):
        """Test Table.insert method"""
        cat = 1
        vals = (cat, 1111, 0.1111, 'test')
        cur = self.connection.cursor()
        self.table.insert(vals, cursor=cur)
        sqlquery = "SELECT cat, cint, creal, ctxt FROM %s WHERE cat=%d"
        cur.execute(sqlquery % (self.tname, cat))
        self.assertTupleEqual(vals, cur.fetchone())

    def test_insert_many(self):
        """Test Table.insert method using many==True"""
        vals = [(1, 1111, 0.1111, 'test1'),
                (2, 2222, 0.2222, 'test2'),
                (3, 3333, 0.3333, 'test3')]
        cur = self.connection.cursor()
        self.table.insert(vals, cursor=cur, many=True)
        sqlquery = "SELECT cat, cint, creal, ctxt FROM %s"
        cur.execute(sqlquery % self.tname)
        self.assertListEqual(vals, cur.fetchall())


class TableUpdateTestCase(DBconnection, TestCase):

    def test_update(self):
        """Test Table.update method"""
        vals = (1122, 0.1122, 'test')
        cat = 1
        cur = self.connection.cursor()
        self.table.update(cat, list(vals), cursor=cur)
        self.connection.commit()
        sqlquery = "SELECT cint, creal, ctxt FROM %s WHERE cat=%d"
        cur.execute(sqlquery % (self.tname, cat))
        self.assertTupleEqual(vals, cur.fetchone())


if __name__ == '__main__':
    test()
