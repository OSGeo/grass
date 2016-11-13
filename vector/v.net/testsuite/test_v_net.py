from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.script.core import read_command


class TestVNet(TestCase):

    network = 'test_vnet'

    def tearDown(cls):
        """Remove viewshed map after each test method"""
        # TODO: eventually, removing maps should be handled through testing framework functions
        cls.runModule('g.remove', flags='f', type='vector',
                      name=cls.network)

    def test_nodes(self):
        """Test"""
        self.assertModule('v.net', input='streets', output=self.network, operation='nodes')
        topology = dict(points=41813, nodes=41813, lines=49746)
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)
        layers = read_command('v.category', input=self.network, option='layers').strip()
        self.assertEqual(first="1", second=layers, msg="Layers do not match")

    def test_nodes_layers(self):
        """Test"""
        self.assertModule('v.net', input='streets', output=self.network, operation='nodes', flags='c')
        topology = dict(points=41813, nodes=41813, lines=49746)
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)
        layers = read_command('v.category', input=self.network, option='layers').strip()
        self.assertEqual(first="1\n2", second=layers, msg="Layers do not match")

    def test_connect(self):
        """Test"""
        self.assertModule('v.net', input='streets', points='schools',
                          output=self.network, operation='connect', threshold=1000)
        topology = dict(points=167, nodes=42136, lines=50069)
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)
        layers = read_command('v.category', input=self.network, option='layers').strip()
        self.assertEqual(first="1\n2", second=layers, msg="Layers do not match")

    def test_connect_snap(self):
        """Test"""
        self.assertModule('v.net', input='streets', points='schools', flags='s',
                          output=self.network, operation='connect', threshold=1000)
        topology = dict(points=167, nodes=41969, lines=49902)
        self.assertVectorFitsTopoInfo(vector=self.network, reference=topology)


if __name__ == '__main__':
    test()
