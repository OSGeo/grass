"""Test of gis library environment management

@author Soeren Gebbert
"""
from grass.gunittest.case import TestCase
import grass.lib.gis as libgis 

class GisLibraryTestEnv(TestCase):

    @classmethod
    def setUpClass(cls):
        libgis.G_gisinit("GisLibraryTestEnv")
        
    def test_gisrc(self):
        # File access
        libgis.G_setenv("TEST", "A");

        value = libgis.G_getenv("TEST")
        self.assertEqual(value, "A")
        value = libgis.G_getenv2("TEST", libgis.G_VAR_GISRC)
        self.assertEqual(value, "A")
        
        # In memory management
        libgis.G_setenv_nogisrc("TEST", "B");

        value = libgis.G_getenv_nofatal("TEST")
        self.assertEqual(value, "B")
        value = libgis.G_getenv_nofatal2("TEST", libgis.G_VAR_GISRC)
        self.assertEqual(value, "B")
        # Force reading
        libgis.G__read_gisrc_env()
        value = libgis.G_getenv("TEST")
        self.assertEqual(value, "A")
        value = libgis.G_getenv2("TEST", libgis.G_VAR_GISRC)
        self.assertEqual(value, "A")

    def test_switch_env(self):
        libgis.G_setenv_nogisrc("TEST", "SWITCH");
        libgis.G_setenv_nogisrc2("TEST", "SWITCH2", libgis.G_VAR_MAPSET);
        # Create alternative env
        libgis.G_create_alt_env()
        libgis.G_setenv_nogisrc("TEST", "TARGET");
        libgis.G_setenv_nogisrc2("TEST", "TARGET2", libgis.G_VAR_MAPSET);
        value = libgis.G_getenv("TEST")
        self.assertEqual(value, "TARGET")
        value = libgis.G_getenv2("TEST", libgis.G_VAR_MAPSET)
        self.assertEqual(value, "TARGET2")
        # Switch back to orig env
        libgis.G_switch_env()
        value = libgis.G_getenv("TEST")
        self.assertEqual(value, "SWITCH")
        value = libgis.G_getenv2("TEST", libgis.G_VAR_MAPSET)
        self.assertEqual(value, "SWITCH2")

    def test_mapset(self):
        # Mapset VAR file
        libgis.G_setenv2("TEST", "C", libgis.G_VAR_MAPSET);
        value = libgis.G_getenv2("TEST", libgis.G_VAR_MAPSET)
        self.assertEqual(value, "C")
 
        libgis.G_setenv_nogisrc2("TEST", "D", libgis.G_VAR_MAPSET);
        value = libgis.G_getenv_nofatal2("TEST", libgis.G_VAR_MAPSET)
        self.assertEqual(value, "D")
        # Force reading
        libgis.G__read_mapset_env()
        value = libgis.G_getenv2("TEST", libgis.G_VAR_MAPSET)
        self.assertEqual(value, "C")
        


if __name__ == '__main__':
    from grass.gunittest.main import test
    test()


