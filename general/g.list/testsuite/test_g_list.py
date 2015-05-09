"""g.list tests"""

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule


class GMlistWrongParamertersTest(TestCase):
    """Test wrong input of parameters for g.list module"""

    @classmethod
    def setUpClass(cls):
        """Create maps in a small region."""
        cls.use_temp_region()
        cls.runModule("g.region", s=0, n=5, w=0, e=5, res=1)

    @classmethod
    def tearDownClass(cls):
        """Remove temporary region"""
        cls.del_temp_region()

    def test_pt_flags(self):
        """Test that -p and -t flags are exclusive"""
        module = SimpleModule('g.list', flags='pt', type='raster')
        self.assertModuleFail(module)
        stderr = module.outputs.stderr
        self.assertIn('-p', stderr)
        self.assertIn('-t', stderr)

    def test_ft_flags(self):
        """Test that -f and -t flags are exclusive"""
        module = SimpleModule('g.list', flags='ft', type='raster')
        self.assertModuleFail(module)
        stderr = module.outputs.stderr
        self.assertIn('-f', stderr)
        self.assertIn('-t', stderr)

    def test_pf_flags(self):
        """Test that -p and -f flags are exclusive"""
        module = SimpleModule('g.list', flags='pf', type='raster')
        self.assertModuleFail(module)
        stderr = module.outputs.stderr
        self.assertIn('-p', stderr)
        self.assertIn('-f', stderr)

    def test_re_flags(self):
        """Test that -r and -e flags are exclusive"""
        module = SimpleModule('g.list', flags='re', type='raster')
        self.assertModuleFail(module)
        stderr = module.outputs.stderr
        self.assertIn('-r', stderr)
        self.assertIn('-e', stderr)


if __name__ == '__main__':
    test()
