"""
Created on Sun Jun 09 11:42:54 2018

@author: Sanjeet Bhatti
"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

from grass.script.core import run_command


class TestVWhatVect(TestCase):
    """Test v.what.vect script"""

    mapName = "myhospitals"

    @classmethod
    def setUpClass(cls):
        """setup"""
        run_command("g.copy", vector="hospitals,myhospitals")

    @classmethod
    def tearDownClass(cls):
        """Remove created vector"""
        cls.runModule("g.remove", type="vector", name=cls.mapName, flags="f")

    def test_what_vect(self):
        """Uploads vector values"""
        run_command("v.db.addcolumn", map=self.mapName, columns="geology_cat integer")

        module = SimpleModule(
            "v.what.vect",
            map=self.mapName,
            query_map="geology",
            column="geology_cat",
            query_column="cat",
        )
        self.assertModule(module)
        minmax = "min=11\nmax=1810"
        self.assertVectorFitsUnivar(
            map=self.mapName, column="geology_cat", reference=minmax
        )


if __name__ == "__main__":
    test()
