from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestProj(TestCase):
    """

    Used dataset: nc_spm_full_v2alpha2

    Test cases:
    test_same_resolution: Test output with region using the same resolution.
    test_different_resolution: Test output with region using a different resolution.
    """

    location = "nc_spm_full_v2alpha2"
    mapset = "PERMANENT"
    to_clear = []
    references = {
        "same": {
            "nearest": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "bilinear": {
                "n": 2022151,
                "null_cells": 2849,
                "cells": 2025000,
                "min": 55.8885459899902,
                "max": 156.329864501953,
                "range": 100.441318511963,
                "mean": 110.399779285869,
                "mean_of_abs": 110.399779285869,
                "stddev": 20.3085640363863,
                "variance": 412.437773220004,
                "coeff_var": 18.3954752153982,
                "sum": 223245024.082699,
            },
            "bicubic": {
                "n": 2016459,
                "null_cells": 8541,
                "cells": 2025000,
                "min": 55.8885459899902,
                "max": 156.329864501953,
                "range": 100.441318511963,
                "mean": 110.411657868429,
                "mean_of_abs": 110.411657868429,
                "stddev": 20.2966017355864,
                "variance": 411.952042013008,
                "coeff_var": 18.3826618741407,
                "sum": 222640581.213715,
            },
            "lanczos": {
                "n": 2013616,
                "null_cells": 11384,
                "cells": 2025000,
                "min": 55.8885459899902,
                "max": 156.329864501953,
                "range": 100.441318511963,
                "mean": 110.398861024319,
                "mean_of_abs": 110.398861024319,
                "stddev": 20.2914098702156,
                "variance": 411.741314521085,
                "coeff_var": 18.3800898686317,
                "sum": 222300912.940346,
            },
            "bilinear_f": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "bicubic_f": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
            "lanczos_f": {
                "n": 2025000,
                "null_cells": 0,
                "cells": 2025000,
                "min": 55.5787925720215,
                "max": 156.329864501953,
                "range": 100.751071929932,
                "mean": 110.375440275606,
                "mean_of_abs": 110.375440275606,
                "stddev": 20.3153233205981,
                "variance": 412.712361620436,
                "coeff_var": 18.4056555243368,
                "sum": 223510266.558102,
            },
        },
        "different": {
            "nearest": {
                "n": 324000,
                "null_cells": 0,
                "cells": 324000,
                "min": 55.9232139587402,
                "max": 156.273727416992,
                "range": 100.350513458252,
                "mean": 110.374542252847,
                "mean_of_abs": 110.374542252847,
                "stddev": 20.3152875302111,
                "variance": 412.710907435151,
                "coeff_var": 18.4057728490259,
                "sum": 35761351.6899223,
            },
            "bilinear": {
                "n": 324000,
                "null_cells": 0,
                "cells": 324000,
                "min": 55.9677429199219,
                "max": 156.230346679688,
                "range": 100.262603759766,
                "mean": 110.375306993838,
                "mean_of_abs": 110.375306993838,
                "stddev": 20.3113089385128,
                "variance": 412.54927079571,
                "coeff_var": 18.4020407206177,
                "sum": 35761599.4660034,
            },
            "bicubic": {
                "n": 321724,
                "null_cells": 2276,
                "cells": 324000,
                "min": 56.0584144592285,
                "max": 156.223129272461,
                "range": 100.164714813232,
                "mean": 110.4044006411,
                "mean_of_abs": 110.4044006411,
                "stddev": 20.2851580571781,
                "variance": 411.487637404699,
                "coeff_var": 18.3735049865636,
                "sum": 35519745.3918571,
            },
            "lanczos": {
                "n": 321724,
                "null_cells": 2276,
                "cells": 324000,
                "min": 56.0589294433594,
                "max": 156.221099853516,
                "range": 100.162170410156,
                "mean": 110.404477059461,
                "mean_of_abs": 110.404477059461,
                "stddev": 20.2851957283322,
                "variance": 411.489165736748,
                "coeff_var": 18.373526390064,
                "sum": 35519769.977478,
            },
            "bilinear_f": {
                "n": 324000,
                "null_cells": 0,
                "cells": 324000,
                "min": 55.9677429199219,
                "max": 156.230346679688,
                "range": 100.262603759766,
                "mean": 110.375306993838,
                "mean_of_abs": 110.375306993838,
                "stddev": 20.3113089385128,
                "variance": 412.54927079571,
                "coeff_var": 18.4020407206177,
                "sum": 35761599.4660034,
            },
            "bicubic_f": {
                "n": 324000,
                "null_cells": 0,
                "cells": 324000,
                "min": 55.9677429199219,
                "max": 156.223129272461,
                "range": 100.255386352539,
                "mean": 110.375287905093,
                "mean_of_abs": 110.375287905093,
                "stddev": 20.3150368008109,
                "variance": 412.700720218302,
                "coeff_var": 18.4054213460164,
                "sum": 35761593.28125,
            },
            "lanczos_f": {
                "n": 324000,
                "null_cells": 0,
                "cells": 324000,
                "min": 55.9677429199219,
                "max": 156.221099853516,
                "range": 100.253356933594,
                "mean": 110.375363786639,
                "mean_of_abs": 110.375363786639,
                "stddev": 20.3150742610671,
                "variance": 412.702242232673,
                "coeff_var": 18.4054426315072,
                "sum": 35761617.8668709,
            },
        },
    }

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_clear:
            cls.runModule(
                "g.remove", flags="f", type="raster", name=",".join(cls.to_clear)
            )

    def test_same_resolution(self):
        self.runModule("g.region", raster="elevation")
        for method, reference in self.references["same"].items():
            output_file = "elevation_same_{}".format(method)
            self.to_clear.append(output_file)
            self.assertModule(
                "r.proj",
                input="elevation",
                location=self.location,
                mapset=self.mapset,
                output=output_file,
                method=method,
            )
            self.assertRasterFitsInfo(
                raster=output_file,
                reference={
                    "north": 228500,
                    "south": 215000,
                    "east": 645000,
                    "west": 630000,
                    "nsres": 10,
                    "ewres": 10,
                    "rows": 1350,
                    "cols": 1500,
                    "cells": 2025000,
                    "datatype": "FCELL",
                    "ncats": 0,
                },
            )
            self.assertRasterFitsUnivar(
                raster=output_file,
                reference=reference,
                precision=1e-5,
            )

    def test_different_resolution(self):
        self.runModule("g.region", raster="elevation", res=25)
        for method, reference in self.references["different"].items():
            output_file = "elevation_diff_{}".format(method)
            self.to_clear.append(output_file)
            self.assertModule(
                "r.proj",
                input="elevation",
                location=self.location,
                mapset=self.mapset,
                output=output_file,
                method=method,
            )
            self.assertRasterFitsInfo(
                raster=output_file,
                reference={
                    "north": 228500,
                    "south": 215000,
                    "east": 645000,
                    "west": 630000,
                    "nsres": 25,
                    "ewres": 25,
                    "rows": 540,
                    "cols": 600,
                    "cells": 324000,
                    "datatype": "FCELL",
                    "ncats": 0,
                },
            )
            self.assertRasterFitsUnivar(
                raster=output_file,
                reference=reference,
                precision=1e-5,
            )


if __name__ == "__main__":
    test()
