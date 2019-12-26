# -*- coding: utf-8 -*-
"""
Created on Fri Feb 26 14:46:06 2016

@author: lucadelu
"""

"""
Test t.rast.import

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author: lucadelu
"""

from grass.gunittest.case import TestCase
import os

class TestRasterImport(TestCase):
    
    input_ = os.path.join("data", "precip_2000.tar.bzip2")
    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", inputs="A")
        
    def test_import(self):
        self.assertModule("t.rast.import", input=self.input_, output="A", 
                          basename="a", overwrite=True)
        tinfo = """start_time='2000-01-01 00:00:00'
                   end_time='2001-01-01 00:00:00'
                   granularity='1 month'
                   map_time=interval
                   north=320000.0
                   south=10000.0
                   east=935000.0
                   west=120000.0
                """
                
        info = SimpleModule("t.info", flags="g", input="A")
        self.assertModuleKeyValue(module=info, reference=tinfo,
                                  precision=2, sep="=")
