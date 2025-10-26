from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestResampInterp(TestCase):
    """

    Used dataset: nc_spm_08_grass7

    Test cases:
    test_finite_options: Test output with finite kernels only.
    test_infinite_box_options: Test output with infinite kernels
        in conjunction with the box kernel.
    """

    test_options = {
        "nearest": {
            "n": 5625000,
            "null_cells": 0,
            "cells": 5625000,
            "min": 55.5787925720215,
            "max": 156.329864501953,
            "range": 100.751071929932,
            "mean": 110.375436614522,
            "mean_of_abs": 110.375436614522,
            "stddev": 20.3153283525752,
            "variance": 412.712566072944,
            "coeff_var": 18.4056606938053,
            "sum": 620861830.956688,
        },
        "bilinear": {
            "n": 5615504,
            "null_cells": 9496,
            "cells": 5625000,
            "min": 55.9232139587402,
            "max": 156.316369018555,
            "range": 100.393155059814,
            "mean": 110.382539288015,
            "mean_of_abs": 110.382539288015,
            "stddev": 20.304736015932,
            "variance": 412.282304676688,
            "coeff_var": 18.3948803378694,
            "sum": 619853590.902052,
        },
        "bicubic": {
            "n": 5601275,
            "null_cells": 23725,
            "cells": 5625000,
            "min": 55.8029828833008,
            "max": 156.356721084961,
            "range": 100.55373820166,
            "mean": 110.404233965113,
            "mean_of_abs": 110.404233965113,
            "stddev": 20.2966878636654,
            "variance": 411.955538235063,
            "coeff_var": 18.3839759896156,
            "sum": 618404475.602926,
        },
        "lanczos": {
            "n": 5596536,
            "null_cells": 28464,
            "cells": 5625000,
            "min": 55.7825176472632,
            "max": 156.36299154775,
            "range": 100.580473900487,
            "mean": 110.396555800561,
            "mean_of_abs": 110.396555800561,
            "stddev": 20.2935834200418,
            "variance": 411.829528026196,
            "coeff_var": 18.3824425253842,
            "sum": 617838298.813768,
        },
    }

    # TODO: replace by unified handing of maps
    to_remove = []

    def check_univar(self, test_case):
        """Checks multiple output against unviariate reference from dict"""
        for method in self.test_options:
            self.assertRasterFitsUnivar(
                raster="{}_raster_{}".format(test_case, method),
                reference=self.test_options[method],
                precision=1e-5,
            )
            self.assertRasterFitsUnivar(
                raster="{}_raster_threaded_{}".format(test_case, method),
                reference=self.test_options[method],
                precision=1e-5,
            )

    @classmethod
    def setUpClass(cls):
        cls.use_temp_region()
        # run with resolution of 6
        cls.runModule("g.region", raster="elevation", res=6)

    @classmethod
    def tearDownClass(cls):
        cls.del_temp_region()
        if cls.to_remove:
            cls.runModule(
                "g.remove", flags="f", type="raster", name=",".join(cls.to_remove)
            )

    def test_resamp_interp(self):
        """Test for all methods of interpolation."""
        test_case = "test_resamp_interp"
        outputs = [
            "{}_raster_{}".format(test_case, filter) for filter in self.test_options
        ]
        outputs_threaded = [
            "{}_raster_threaded_{}".format(test_case, filter)
            for filter in self.test_options
        ]
        self.to_remove.extend(outputs)
        self.to_remove.extend(outputs_threaded)

        for method in self.test_options:
            self.assertModule(
                "r.resamp.interp",
                input="elevation",
                output="{}_raster_{}".format(test_case, method),
                method=method,
            )
            self.assertModule(
                "r.resamp.interp",
                input="elevation",
                output="{}_raster_threaded_{}".format(test_case, method),
                method=method,
                nprocs=8,
            )
        self.check_univar(test_case)


if __name__ == "__main__":
    test()
