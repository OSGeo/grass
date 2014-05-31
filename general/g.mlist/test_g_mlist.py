
import unittest
from grass.script import start_command
import subprocess

class GMlistWrongParamertersTest(unittest.TestCase):

    def setUp(self):
        pass

    def test_pt_flags(self):
        self.maxDiff = None
        p = start_command('g.mlist', flags='pt', type='rast', stderr=subprocess.PIPE)
        stderr = p.communicate()[1]
        self.assertEqual(stderr, "ERROR: -p/-f and -t are mutually exclusive\n")

    def test_ft_flags(self):
        self.maxDiff = None
        p = start_command('g.mlist', flags='ft', type='rast', stderr=subprocess.PIPE)
        stderr = p.communicate()[1]
        self.assertEqual(stderr, "ERROR: -p/-f and -t are mutually exclusive\n")

    def test_pf_flags(self):
        self.maxDiff = None
        p = start_command('g.mlist', flags='pf', type='rast', stderr=subprocess.PIPE)
        stderr = p.communicate()[1]
        self.assertEqual(stderr, "ERROR: -p and -f are mutually exclusive\n")

    def test_re_flags(self):
        self.maxDiff = None
        p = start_command('g.mlist', flags='re', type='rast', stderr=subprocess.PIPE)
        stderr = p.communicate()[1]
        self.assertEqual(stderr, "ERROR: -r and -e are mutually exclusive\n")

    def tearDown(self):
        pass

if __name__ == '__main__':
    unittest.main()
