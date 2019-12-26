"""Test t.vector.extract

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

:authors: Luca Delucchi
"""

from grass.gunittest.case import TestCase
from grass.gunittest.gmodules import SimpleModule

class TestVectorExtraction(TestCase):

    @classmethod
    def setUpClass(cls):
        """Initiate the temporal GIS and set the region
        """
        cls.use_temp_region()
         
        cls.runModule("g.region", s=0, n=80, w=0, e=120, res=10)
        # Use always the current mapset as temporal database
        for i in range(1, 11):
            cls.runModule("v.random", output="a{c}".format(c=i), npoints=20,
                          overwrite=True)
            cls.runModule("v.db.addtable", map="a{c}".format(c=i),
                          columns="value integer")
            cls.runModule("v.db.update",  map="a{c}".format(c=i),
                          column="value", value="'random()'")
        # Create the temporal database
        cls.runModule("t.connect", flags="d")
        cls.runModule("t.info", flags="d")
        cls.runModule("t.create", type="stvds", temporaltype="absolute",
                      output="A", title="A testvect", description="A testvect",
                      overwrite=True)
        cls.runModule("t.register", flags="i", type="vector", input="A",
                      maps="a1,a2,a3,a4,a5,a6,a7,a8,a9,a10", start="2001-01-01",
                      increment="3 months", overwrite=True)

    @classmethod
    def tearDownClass(cls):
        """Remove the temporary region
        """
        cls.del_temp_region()
        cls.runModule("t.remove", flags="rf", type="stvds", inputs="A")
        
    def test_selection(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.vect.extract", input="A", output="B", 
                          where="start_time > '2001-06-01'", overwrite=True)

        tinfo_string="""start_time='2001-07-01 00:00:00'
        end_time='2003-07-01 00:00:00'
        granularity='3 months'
        map_time=interval
        number_of_maps=8
        primitives=160
        points=160"""

        info = SimpleModule("t.info", flags="g", type="stvds", input="B")
        self.assertModuleKeyValue(module=info, reference=tinfo_string,
                                  precision=2, sep="=")

    def test_selection_no_suffix(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.vect.extract", input="A", output="B", 
                          where="start_time > '2001-06-01'", basename="b",
                          overwrite=True)
        self.assertVectorDoesNotExist('b_2001_07')
        self.runModule("t.remove", flags="rf", type="stvds", inputs="B")

    def test_selection_suffix(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.vect.extract", input="A", output="B", 
                          expression="value > 0", basename="b",
                          overwrite=True)
        self.assertVectorDoesNotExist('b_2001_07')
        self.runModule("t.remove", flags="rf", type="stvds", inputs="B")

    def test_selection_time_suffix(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.vect.extract", input="A", output="B", 
                          expression="value > 0", basename="b", suffix="time",
                          overwrite=True)
        self.assertVectorExists('b_2001_01_01T00_00_00')
        self.runModule("t.remove", flags="rf", type="stvds", inputs="B")

    def test_selection_num_suffix(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.vect.extract", input="A", output="B", 
                          expression="value > 0", basename="b", suffix="num",
                          overwrite=True)
        self.assertVectorExists('b_00001')
        self.runModule("t.remove", flags="rf", type="stvds", inputs="B")

    def test_selection_num3_suffix(self):
        """Perform a simple selection by datetime"""
        self.assertModule("t.vect.extract", input="A", output="B", 
                          expression="value > 0", basename="b",
                          suffix="num%03", overwrite=True)
        self.assertVectorExists('b_001')
        self.runModule("t.remove", flags="rf", type="stvds", inputs="B")

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
