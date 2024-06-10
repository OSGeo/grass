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
        values = """min=0.104167
        max=1
        mean=0.112871783083256
        variance=0.000158101620160753
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="asm", method="asm")
        self.assertRasterFitsUnivar(asm_ASM, reference=values, precision=1e-2)

    def test_contrast(self):
        """Testing method contrast"""
        corr_output = "contrast_Contr"
        values = """min=0
        max=14680.771484375
        mean=176.569350873014
        variance=185767.349118713
        cells=1000246"""
        self.assertModule(
            "r.texture", input=self.input, output="contrast", method="contrast"
        )
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=1e-2)

    def test_corr(self):
        """Testing method corr"""
        corr_output = "corr_Corr"
        values = """min=-0.533553719520569
        max=0.443134188652039
        mean=-0.0511981885382368
        variance=0.0306650179549719
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="corr", method="corr")
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=1e-2)

    def test_var(self):
        """Testing method var"""
        corr_output = "var_Var"
        values = """min=0
        max=8776.787109375
        mean=90.3218668390769
        variance=58439.1071892438
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="var", method="var")
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=1e-2)

    def test_idm(self):
        """Testing method idm"""
        corr_output = "idm_IDM"
        values = """min=0.000490661768708378
        max=1
        mean=0.12308521457551
        variance=0.00497110161251246
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="idm", method="idm")
        self.assertRasterFitsUnivar(corr_output, reference=values, precision=1e-2)

    def test_sa(self):
        """Testing method sa"""
        sa_SA = "sa_SA"
        values = """min=95.9583358764648
        max=2942.79150390625
        mean=742.367934271396
        variance=31616.3527726298
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="sa", method="sa")
        self.assertRasterFitsUnivar(sa_SA, reference=values, precision=1e-2)

    def test_sv(self):
        """Testing method sv"""
        sv_SV = "sv_SV"
        values = """min=0
        max=45368492
        mean=2248724.35829364
        variance=2332049431762.5
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="sv", method="sv")
        self.assertRasterFitsUnivar(sv_SV, reference=values, precision=1e-2)

    def test_se(self):
        """Testing method se"""
        se_SE = "se_SE"
        values = """min=0
        max=2.29248142242432
        mean=1.99162619886133
        variance=0.0255830167563798
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="se", method="se")
        self.assertRasterFitsUnivar(se_SE, reference=values, precision=1e-2)

    def test_entr(self):
        """Testing method entr"""
        entr_Entr = "entr_Entr"
        values = """min=-0
        max=3.29248118400574
        mean=3.20825728104855
        variance=0.0106857128131089
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="entr", method="entr")
        self.assertRasterFitsUnivar(entr_Entr, reference=values, precision=1e-2)

    def test_dv(self):
        """Testing method dv"""
        dv_DV = "dv_DV"
        values = """min=-4.33281384175643e-05
        max=0.115451395511627
        mean=0.00110390526476111
        variance=2.37568891441793e-06
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="dv", method="dv")
        self.assertRasterFitsUnivar(dv_DV, reference=values, precision=1e-2)

    def test_de(self):
        """Testing method de"""
        de_DE = "de_DE"
        values = """min=-0
        max=2.29248142242432
        mean=1.6115293021953
        variance=0.055056566141371
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="de", method="de")
        self.assertRasterFitsUnivar(de_DE, reference=values, precision=1e-2)

    def test_moc1(self):
        """Testing method moc1"""
        moc1_MOC1 = "moc1_MOC-1"
        values = """min=-0.910445094108582
        max=0
        mean=-0.782939935071241
        variance=0.00736236364469843
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="moc1", method="moc1")
        self.assertRasterFitsUnivar(moc1_MOC1, reference=values, precision=1e-2)

    def test_moc2(self):
        """Testing method moc2"""
        moc2_MOC2 = "moc2_MOC-2"
        values = """min=-0
        max=0.996889352798462
        mean=0.98765725693893
        variance=0.000334346005602857
        cells=1000246"""
        self.assertModule("r.texture", input=self.input, output="moc2", method="moc2")
        self.assertRasterFitsUnivar(moc2_MOC2, reference=values, precision=1e-2)


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
