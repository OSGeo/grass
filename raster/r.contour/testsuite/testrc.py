"""
Name:       r.contour test
Purpose:    Tests r.contour and its flags/options.
    
Author:     Sunveer Singh, Google Code-in 2018
Copyright:  (C) 2018 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
                License (>=v2). Read the file COPYING that comes with GRASS
                for details.
"""
from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.gunittest.gmodules import SimpleModule

out_where = u"""cat|level
1|56
2|58
3|60
4|62
5|64
6|66
7|68
8|70
9|72
10|74
11|76
12|78
13|80
14|82
15|84
16|86
17|88
18|90
19|92
20|94
21|96
22|98
23|100
24|102
25|104
26|106
27|108
28|110
29|112
30|114
31|116
32|118
33|120
34|122
35|124
36|126
37|128
38|130
39|132
40|134
41|136
42|138
43|140
44|142
45|144
46|146
47|148
48|150
49|152
50|154
51|156
"""

class Testrr(TestCase):
    input='elevation'
    output='towns'

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule('g.region', raster=cls.input)
    
    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()


    def tearDown(cls):
        cls.runModule('g.remove', type='vector', flags='f', name=cls.output)


    def test_flag_t(self):
        """Testing flag t"""
        string=u"""min=1
        max=6"""
        self.assertModule('r.contour', input=self.input, output=self.output, levels=1, step=1, flags='t')
        self.assertRasterFitsUnivar(self.output,
                                reference=string, precision=2)

    def test_vector(self):
        """Testing vector output"""
        self.assertModule('r.contour', input=self.input, output=self.output, step=5, flags='t')
        self.assertModule('v.info', map=self.output, flags='t')
        topology = dict(points=0, lines=2222, areas=0)
        self.assertVectorFitsTopoInfo(self.output, topology)

    def test_v_db_select(self):
        """Testing attribute values of contours with v.db.select """
        self.assertModule('r.contour', input=self.input, output=self.output, step=2)
        v_db_select = SimpleModule('v.db.select', map=self.output)
        v_db_select.run()
        self.assertLooksLike(reference=out_where, actual=v_db_select.outputs.stdout)

if __name__ == '__main__':
    from grass.gunittest.main import test
    test()

