"""
Name:       r.texture test
Purpose:    Tests r.texture and its flags/options.
	
Author:     Sunveer Singh, Google Code-in 2017
Copyright:  (C) 2017 by Sunveer Singh and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
	            License (>=v2). Read the file COPYING that comes with GRASS
	            for details.
"""
from grass.gunittest.case import TestCase
class TestRasterreport(TestCase):
    input = "lsat7_2002_80"
   
    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        cls.runModule("g.region", raster=cls.input)
	
    @classmethod
    def tearDownClass(cls): 
        cls.del_temp_region()
        cls.runModule('g.remove', flags='f', type='raster', name="asm_ASM")
        cls.runModule('g.remove', flags='f', type='raster', name='corr_Corr')
        cls.runModule('g.remove', flags='f', type='raster', name='sa_SA')
        cls.runModule('g.remove', flags='f', type='raster', name='var_Var')
        cls.runModule('g.remove', flags='f', type='raster', name='idm_IDM')
        
    def test_asm(self):
        """Testing method asm"""
        asm_ASM='asm_ASM'
        string="""min=1
        max=1
        cells=1000246"""
        self.assertModule('r.texture', input=self.input, output='asm', method='asm')
        self.assertRasterFitsUnivar(asm_ASM,
	                            reference=string, precision=2)

    def test_corr(self):
        """Testing method corr"""
        corr_output='corr_Corr'
        string="""min=1
        max=1
        cells=1000246"""
        self.assertModule('r.texture', input=self.input, output='corr', method='corr')
        self.assertRasterFitsUnivar(corr_output,
	                            reference=string, precision=2)

    def test_sa(self):
        """Testing method sa"""
        sa_SA='sa_SA'
        string="""min=95.95
        max=2942.79
        cells=1000246"""
        self.assertModule('r.texture', input=self.input, output='sa', method='sa')
        self.assertRasterFitsUnivar(sa_SA,
	                            reference=string, precision=2)

    def test_var(self):
        """Testing method var"""
        var_output='var_Var'
        self.assertModule('r.texture', input=self.input, output='var', method='var')
        self.assertRasterMinMax(map=var_output, refmin=0, refmax=9843.07,
	                        msg="var_Var in degrees must be between 0.74 and 9843.07")

    def test_idm(self):
        """Testing method idm"""
        idm_IDM='idm_IDM'
        self.assertModule('r.texture', input=self.input, output='idm', method='idm')
        self.assertRasterMinMax(map=idm_IDM, refmin=0, refmax=9843.07,
	                        msg="idm_IDM_IDM in degrees must be between 0.74 and 9843.07")
      
if __name__ == '__main__':
    from grass.gunittest.main import test
    test()
