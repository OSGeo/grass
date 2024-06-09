"""
Name:       r.texture test
Purpose:    Tests r.texture and its flags/options.

Author:     Sunveer Singh, Google Code-in 2017
            Chung-Yuan Liang, modified in 2024
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
        cls.runModule("g.remove", flags="f", type="raster", name="asm_ASM")
        cls.runModule("g.remove", flags="f", type="raster", name="contrast_Contr")
        cls.runModule("g.remove", flags="f", type="raster", name="corr_Corr")
        cls.runModule("g.remove", flags="f", type="raster", name="var_Var")
        cls.runModule("g.remove", flags="f", type="raster", name="idm_IDM")
        cls.runModule("g.remove", flags="f", type="raster", name="var_Var")
        cls.runModule("g.remove", flags="f", type="raster", name="sa_SA")
        cls.runModule("g.remove", flags="f", type="raster", name="sv_SV")
        cls.runModule("g.remove", flags="f", type="raster", name="se_SE")
        cls.runModule("g.remove", flags="f", type="raster", name="entr_Entr")
        cls.runModule("g.remove", flags="f", type="raster", name="dv_DV")
        cls.runModule("g.remove", flags="f", type="raster", name="de_DE")
        cls.runModule("g.remove", flags="f", type="raster", name="moc1_MOC-1")
        cls.runModule("g.remove", flags="f", type="raster", name="moc2_MOC-2")

    def test_asm(self):
        """Testing method asm"""
        asm_ASM = "asm_ASM"
        values = """min=-0.0455724261701107
        max=0.0380486063659191
        mean=-0.000136686790876467
        variance=0.000158102
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="asm", method="asm")
        self.assertRasterFitsUnivar(asm_ASM, reference=values, precision=2)

    def test_contrast(self):
        """Testing method contrast"""
        corr_output = "contrast_Contr"
        values = """min=0
        max=14680.8
        mean=176.569
        variance=185767
        cells=1000246"""
        self.assertModule(
            "r.texture", input=self.input, output="contrast", method="contrast"
        )
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=2)

    def test_corr(self):
        """Testing method corr"""
        corr_output = "corr_Corr"
        values = """min=-0.533554
        max=0.443134
        mean=-0.0511982
        variance=0.030665
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="corr", method="corr")
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=2)

    def test_var(self):
        """Testing method var"""
        corr_output = "var_Var"
        values = """min=0
        max=8776.79
        mean=90.3219
        variance=58439.1
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="var", method="var")
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=2)

    def test_idm(self):
        """Testing method idm"""
        corr_output = "idm_IDM"
        values = """min=0.000490662
        max=1
        mean=0.123085
        variance=0.0049711
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="idm", method="idm")
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=2)

    def test_sa(self):
        """Testing method sa"""
        sa_SA = "sa_SA"
        values = """min=95.9583
        max=2942.79
        mean=742.368
        variance=31616.4
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="sa", method="sa")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_sv(self):
        """Testing method sv"""
        sa_SA = "sv_SV"
        values = """min=0
        max=45368492
        mean=2248724.35829364
        variance=2332049431762.5
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="sv", method="sv")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_se(self):
        """Testing method se"""
        sa_SA = "se_SE"
        values = """min=0
        max=2.29248
        mean=1.99163
        variance=0.025583
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="se", method="se")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_entr(self):
        """Testing method entr"""
        sa_SA = "entr_Entr"
        values = """min=-0
        max=3.29248
        mean=3.20826
        variance=0.0106857
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="entr", method="entr")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_dv(self):
        """Testing method dv"""
        sa_SA = "dv_DV"
        values = """min=-4.33281e-05
        max=0.115451
        mean=0.00110391
        variance=2.37569e-06
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="dv", method="dv")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_de(self):
        """Testing method de"""
        sa_SA = "de_DE"
        values = """min=-0
        max=2.29248
        mean=1.61153
        variance=0.0550566
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="de", method="de")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_moc1(self):
        """Testing method moc1"""
        sa_SA = "moc1_MOC-1"
        values = """min=-0.910445
        max=0
        mean=-0.78294
        variance=0.00736236
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="moc1", method="moc1")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)

    def test_moc2(self):
        """Testing method moc2"""
        sa_SA = "moc2_MOC-2"
        values = """min=-0
        max=0.996889
        mean=0.987657
        variance=0.000334346
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="moc2", method="moc2")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=2)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
