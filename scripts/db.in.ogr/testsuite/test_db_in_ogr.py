"""
Created on Sun Jun 07 20:14:04 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command
from grass.script.utils import decode

import os


class TestDbInOgr(TestCase):
    """Test db.in.ogr script"""

    csvFile = 'sample_data.csv'
    dbfFile = 'sample_data.dbf'
    tableName1 = 'sample_table1'
    tableName2 = 'sample_table2'

    @classmethod
    def setUpClass(cls):
        """Create temporary files. Remove if the tables already exists."""
        cls.runModule('db.out.ogr', input='geology', output=cls.csvFile)
        cls.runModule('db.out.ogr', input='geology', output=cls.dbfFile,
                      format='DBF')
        cls.runModule('db.droptable', table=cls.tableName1, flags='f')
        cls.runModule('db.droptable', table=cls.tableName2, flags='f')

    @classmethod
    def tearDownClass(cls):
        """Remove the created files and the created table"""
        os.remove(cls.csvFile)
        os.remove(cls.dbfFile)
        cls.runModule('db.droptable', table=cls.tableName1, flags='f')
        cls.runModule('db.droptable', table=cls.tableName2, flags='f')

    def test_import_csv_file(self):
        """import csv table"""
        module = SimpleModule('db.in.ogr', input=self.csvFile,
                              output=self.tableName1)
        self.assertModule(module)

        m = SimpleModule('db.tables', flags='p')
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), self.tableName1)

    def test_import_dbf_file(self):
        """import dbf table"""
        module = SimpleModule('db.in.ogr', input=self.dbfFile,
                              output=self.tableName2)
        self.assertModule(module)

        m = SimpleModule('db.tables', flags='p')
        self.assertModule(m)
        self.assertRegexpMatches(decode(m.outputs.stdout), self.tableName2)

if __name__ == '__main__':
    test()
