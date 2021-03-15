#!/usr/bin/env python3

from grass.gunittest.case import TestCase
from grass.gunittest.main import test

output = 'test01'
output2 = 'test02'
npoints = 100
state = 'boundary_state'
zmin=10
zmax=120

class Test_v_random(TestCase):

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', vector=state)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()

    def tearDown(cls):
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output)
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output2)



    #Checking if number of points equals 100
    def test_num_points(self):
        self.assertModule('v.random', output=output, npoints=npoints)

        topology = dict(points=npoints)
        self.assertVectorFitsTopoInfo(vector=output, reference=topology)

    #Checking if the map is 3D and number of points is 100
    def test_num_points_3D(self):
        self.assertModule('v.random', output=output, npoints=npoints,
                          zmin=zmin, zmax=zmax, flags='z')

        topology = dict(points=npoints, map3d=1)
        self.assertVectorFitsTopoInfo(vector=output, reference=topology)

    #Checking if all points are in the polygon boundary state
    def test_restrict(self):
        self.assertModule('v.random', output=output, npoints=npoints,
                          restrict=state)
        self.assertModule('v.clip', input=output, clip=state, output=output2)

        self.assertVectorInfoEqualsVectorInfo(output,output2,precision=0.01)


if __name__ == '__main__':
    test()
