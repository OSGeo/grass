# -*- coding: utf-8 -*-
"""
Created on Thu Jul  2 07:25:34 2015

@author: pietro
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test

from grass.pygrass.vector.table import Filters


class FiltersTestCase(TestCase):

    def setUp(self):
        """Create a not empty filters instance"""
        self.filters = Filters('table')
        self.attrs = ('_select', '_where', '_groupby', '_orderby', '_limit')

    def test_init(self):
        """Test Filters __init__"""
        self.assertEqual(self.filters.tname, 'table')
        for attr in self.attrs:
            self.assertEqual(getattr(self.filters, attr), None)

    def test_select(self):
        """Test Filters select method"""
        self.assertEqual(self.filters.select()._select,
                         'SELECT * FROM table')
        self.assertEqual(self.filters.select('column')._select,
                         'SELECT column FROM table')
        self.assertEqual(self.filters.select('column0', 'column1')._select,
                         'SELECT column0, column1 FROM table')

    def test_where(self):
        """Test Filters where method"""
        self.assertEqual(self.filters.where('column >= 10')._where,
                         'WHERE column >= 10')

    def test_order_by(self):
        """Test Filters order_by method"""
        self.assertEqual(self.filters.order_by('column')._orderby,
                         'ORDER BY column')
        self.assertEqual(self.filters.order_by('column0', 'column1')._orderby,
                         'ORDER BY column0, column1')

    def test_limit(self):
        """Test Filters limit method"""
        self.assertEqual(self.filters.limit(10)._limit, 'LIMIT 10')
        with self.assertRaises(ValueError):
            self.filters.limit('a')

    def test_group_by(self):
        """Test Filters group_by method"""
        self.assertEqual(self.filters.group_by('column')._groupby,
                         'GROUP BY column')
        self.assertEqual(self.filters.group_by('column0', 'column1')._groupby,
                         'GROUP BY column0, column1')

    def test_get_sql(self):
        """Test Filters get_sql method"""
        sql_code = 'select_sql'
        sep = '=> '
        fsep = ' ' + sep
        self.filters._select = sql_code
        for attr in self.attrs[1:]:
            attr_sql = sep + attr[1:]
            setattr(self.filters, attr, attr_sql)
            sql_code += fsep + attr[1:]
            self.assertEqual(self.filters.get_sql(), sql_code + ';')

    def test_reset(self):
        """Test Filters reset method"""
        # fill the attributes with strigns
        for attr in self.attrs:
            setattr(self.filters, attr, attr)
        self.filters.reset()
        for attr in self.attrs:
            self.assertEqual(getattr(self.filters, attr), None)

if __name__ == '__main__':
    test()
