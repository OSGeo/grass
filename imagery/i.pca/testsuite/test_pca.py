"""
Name:       i.pca test
Purpose:    Tests i.pca.

Author:     Hamed Elgizery - hamedashraf2004@gmail.com
Copyright:  (C) 2024 by Hamed Elgizery and the GRASS Development Team
Licence:    This program is free software under the GNU General Public
                License (>=v2). Read the file COPYING that comes with GRASS
                for details.
"""

from grass.gunittest.case import TestCase


class TestReport(TestCase):
    @classmethod
    def setUpClass(cls):
        """Use temporary region settings"""
        cls.runModule("g.region", raster="lsat7_2002_10@PERMANENT")
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        cls.runModule(
            "g.remove",
            flags="f",
            type="raster",
            name="lsat7_2002_pca.1,lsat7_2002_pca.2,lsat7_2002_pca.3,lsat7_2002_pca.4,lsat7_2002_pca.6",
        )
        cls.del_temp_region()

    def test_pca_sample(self):
        """Testing pca sample"""
        self.assertModule(
            "i.pca",
            input="lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40,lsat7_2002_50,lsat7_2002_70",
            output="lsat7_2002_pca",
        )

        lsat7_2002_pca_info_out = """north=228513
            south=214975.5
            east=645012
            west=629992.5
            nsres=28.5
            ewres=28.5
            rows=475
            cols=527
            cells=250325
            datatype=CELL
            ncats=0
            comments=\"Eigen values, (vectors), and [percent importance]:PC1   4334.35 ( 0.2824, 0.3342, 0.5092,-0.0087, 0.5264, 0.5217) [83.04%]PC2    588.31 ( 0.2541, 0.1885, 0.2923,-0.7428,-0.5110,-0.0403) [11.27%]PC3    239.22 ( 0.3801, 0.3819, 0.2681, 0.6238,-0.4000,-0.2980) [ 4.58%]PC4     32.85 ( 0.1752,-0.0191,-0.4053, 0.1593,-0.4435, 0.7632) [ 0.63%]PC5     20.73 (-0.6170,-0.2514, 0.6059, 0.1734,-0.3235, 0.2330) [ 0.40%]PC6      4.08 (-0.5475, 0.8021,-0.2282,-0.0607,-0.0208, 0.0252) [ 0.08%]i.pca input=\"lsat7_2002_10,lsat7_2002_20,lsat7_2002_30,lsat7_2002_40\\,lsat7_2002_50,lsat7_2002_70\" output=\"lsat7_2002_pca\" rescale=0,255 \\percent=99" """

        lsat7_2002_pca_univar_out = [
            """n=250325
            null_cells=0
            cells=250325
            min=0
            max=255
            range=255
            mean=60.6958074503146
            mean_of_abs=60.6958074503146
            stddev=32.8850846003739
            variance=1081.42878917375
            coeff_var=54.1801583697417
            sum=15193678
            first_quartile=36
            median=51
            third_quartile=77
            percentile_90=101""",
            """n=250325
            null_cells=0
            cells=250325
            min=0
            max=255
            range=255
            mean=106.099418755618
            mean_of_abs=106.099418755618
            stddev=26.4487056926998
            variance=699.534032819051
            coeff_var=24.928228639612
            sum=26559337
            first_quartile=88
            median=104
            third_quartile=121
            percentile_90=137""",
            """n=250325
            null_cells=0
            cells=250325
            min=0
            max=255
            range=255
            mean=74.1768980325577
            mean_of_abs=74.1768980325577
            stddev=14.1956266450161
            variance=201.515815844691
            coeff_var=19.1375307158104
            sum=18568332
            first_quartile=67
            median=74
            third_quartile=81
            percentile_90=88""",
            """n=250325
            null_cells=0
            cells=250325
            min=0
            max=255
            range=255
            mean=113.285145311096
            mean_of_abs=113.285145311096
            stddev=10.689092045444
            variance=114.256688755974
            coeff_var=9.43556369733241
            sum=28358104
            first_quartile=109
            median=114
            third_quartile=118
            percentile_90=122""",
            """n=250325
            null_cells=0
            cells=250325
            min=0
            max=255
            range=255
            mean=110.346713272745
            mean_of_abs=110.346713272745
            stddev=8.43087149474902
            variance=71.0795941609716
            coeff_var=7.64034672596938
            sum=27622541
            first_quartile=106
            median=110
            third_quartile=114
            percentile_90=118""",
            """n=250325
            null_cells=0
            cells=250325
            min=0
            max=255
            range=255
            mean=115.238465994208
            mean_of_abs=115.238465994208
            stddev=8.97064489504434
            variance=80.4724698329851
            coeff_var=7.78441887233665
            sum=28847069
            first_quartile=110
            median=115
            third_quartile=121
            percentile_90=126""",
        ]

        for i in range(1, 7):
            # Asserting the results givien from r.info
            self.assertRasterFitsInfo(
                raster=f"lsat7_2002_pca.{i}",
                reference=lsat7_2002_pca_info_out,
                precision=3,
            )

            # Asserting the results givien from r.univar
            univar_out = lsat7_2002_pca_univar_out[i - 1]
            self.assertModuleKeyValue(
                "r.univar",
                flags="eg",
                map=f"lsat7_2002_pca.{i}",
                reference=univar_out,
                precision=3,
                sep="=",
            )


if __name__ == "__main__":
    from grass.gunittest.main import test

    test()
