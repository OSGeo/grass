
import unittest
import subprocess
from grass.script import start_command, read_command, run_command
import grass.script.core as gcore
from grass.script.raster import mapcalc as rmapcalc

REMOVE_RASTERS = """rast/test_map_0@user1
rast/test_map_1@user1
rast/test_map_2@user1
rast/test_map_3@user1
rast/test_map_4@user1
rast/test_map_5@user1
rast/test_map_6@user1
rast/test_map_7@user1
rast/test_map_8@user1
rast/test_map_9@user1
rast/test_two@user1
"""

REMOVING_RASTERS_LOG = """Removing raster <test_map_0>
Removing raster <test_map_1>
Removing raster <test_map_2>
Removing raster <test_map_3>
Removing raster <test_map_4>
Removing raster <test_map_5>
Removing raster <test_map_6>
Removing raster <test_map_7>
Removing raster <test_map_8>
Removing raster <test_map_9>
Removing raster <test_two>
"""


class GMremoveTest(unittest.TestCase):

    def setUp(self):
        gcore.set_raise_on_error(True)
        gcore.use_temp_region()
        ret = run_command("g.region", s=0, n=5, w=0, e=5, res=1)
        if ret != 0:
            gcore.fatal("g.region failed")

    def test_remove_procedure(self):
        self.maxDiff = None

        for i in range(0, 10):
            rmapcalc("test_map_%i = 100" % i)
        rmapcalc("test_two = 2")
        p = start_command('g.mremove', rast='test_map_*,*two',
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout = p.communicate()[0]
        self.assertMultiLineEqual(stdout, REMOVE_RASTERS)
        p = start_command('g.mremove', rast='test_map_*,*two', flags='f',
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        self.assertMultiLineEqual(stdout, '')
        self.assertMultiLineEqual(stderr, REMOVING_RASTERS_LOG)

    #@unittest.skip("only for the new g.mremove with g.mlist interface")
    def test_remove_procedure(self):
        self.maxDiff = None

        rmapcalc("test_apples = 100")
        rmapcalc("test_oranges = 200")
        rmapcalc("test_apples_big = 300")
        rmapcalc("test_apples_small = 300")
        p = start_command('g.mremove', type='rast',
                          pattern='test_{apples,oranges}*',
                          exclude="*_small",
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout = p.communicate()[0]
        self.assertMultiLineEqual(stdout,
                                  'rast/test_apples@user1\n'
                                  'rast/test_apples_big@user1\n'
                                  'rast/test_oranges@user1\n')
        p = start_command('g.mremove', type='rast',
                          pattern='test_{apples,oranges}{_small,_big,*}',
                          flags='f',
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        self.assertMultiLineEqual(stdout, '')
        self.assertRegexpMatches(stderr, "(.*<.+>[^\n]*\n){4}", "4 maps should be removed")

    def test_re_flags(self):
        p = start_command('g.mremove', flags='re', type='rast', pattern='xxxyyyzzz',
                          stderr=subprocess.PIPE)
        stderr = p.communicate()[1]
        self.assertEqual(stderr, "ERROR: -r and -e are mutually exclusive\n")

    def tearDown(self):
        gcore.del_temp_region()

if __name__ == '__main__':
    unittest.main()
