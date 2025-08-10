from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestResampFilter(TestCase):
    """

    Used dataset: nc_spm_08_grass7

    Test cases:
    test_finite_options: Test output with finite kernels only.
    test_infinite_box_options: Test output with infinite kernels
        in conjunction with the box kernel.
    """

    test_options = {
        "test_finite_options": {
            "box": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6378765106201,
                "max": 156.305896759033,
                "range": 100.668020248413,
                "mean": 110.375439282307,
                "mean_of_abs": 110.375439282307,
                "stddev": 20.3084660266678,
                "variance": 412.433792356322,
                "coeff_var": 18.3994429908677,
                "sum": 620861845.962989,
            },
            "bartlett": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6260597229004,
                "max": 156.316369018555,
                "range": 100.690309295654,
                "mean": 110.375440204059,
                "mean_of_abs": 110.375440204059,
                "stddev": 20.3119057626464,
                "variance": 412.573515710628,
                "coeff_var": 18.4025592333715,
                "sum": 620861851.147877,
            },
            "hermite": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6203876647949,
                "max": 156.322196495117,
                "range": 100.701808830322,
                "mean": 110.375440282578,
                "mean_of_abs": 110.375440282578,
                "stddev": 20.3125721916948,
                "variance": 412.600589042813,
                "coeff_var": 18.403163004099,
                "sum": 620861851.589519,
            },
            "lanczos1": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6023817623363,
                "max": 156.325354289019,
                "range": 100.722972526682,
                "mean": 110.375440231786,
                "mean_of_abs": 110.375440231786,
                "stddev": 20.313022054599,
                "variance": 412.618864990624,
                "coeff_var": 18.4035705877522,
                "sum": 620861851.30377,
            },
            "lanczos2": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.4287505083574,
                "max": 156.36299154775,
                "range": 100.934241039393,
                "mean": 110.375435152297,
                "mean_of_abs": 110.375435152297,
                "stddev": 20.3151009676525,
                "variance": 412.703327325917,
                "coeff_var": 18.405454927196,
                "sum": 620861822.731579,
            },
            "lanczos3": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.4381170630086,
                "max": 156.356150645782,
                "range": 100.918033582773,
                "mean": 110.375439667324,
                "mean_of_abs": 110.375439667324,
                "stddev": 20.3154206838383,
                "variance": 412.716317561325,
                "coeff_var": 18.4057438367356,
                "sum": 620861848.128782,
            },
        },
        "test_infinite_box_options": {
            "gauss": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6262147993191,
                "max": 156.314719743192,
                "range": 100.688504943873,
                "mean": 110.375440194786,
                "mean_of_abs": 110.375440194786,
                "stddev": 20.3108768847555,
                "variance": 412.531719827694,
                "coeff_var": 18.4016270729355,
                "sum": 620861851.095697,
            },
            "normal": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6349247730616,
                "max": 156.307900724069,
                "range": 100.672975951008,
                "mean": 110.375439623437,
                "mean_of_abs": 110.375439623437,
                "stddev": 20.3089516110606,
                "variance": 412.4535155404,
                "coeff_var": 18.3998828728091,
                "sum": 620861847.881879,
            },
            "sinc": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6260597229004,
                "max": 156.316369018555,
                "range": 100.690309295654,
                "mean": 110.375440204059,
                "mean_of_abs": 110.375440204059,
                "stddev": 20.3119057626423,
                "variance": 412.573515710463,
                "coeff_var": 18.4025592333678,
                "sum": 620861851.147877,
            },
            "hann": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6170862149874,
                "max": 156.32277090836,
                "range": 100.705684693373,
                "mean": 110.375440282543,
                "mean_of_abs": 110.375440282543,
                "stddev": 20.3126467418446,
                "variance": 412.603617658969,
                "coeff_var": 18.4032305464401,
                "sum": 620861851.589254,
            },
            "hamming": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.6223234496647,
                "max": 156.318925911936,
                "range": 100.696602462271,
                "mean": 110.375440209185,
                "mean_of_abs": 110.375440209185,
                "stddev": 20.3116372028825,
                "variance": 412.562605861522,
                "coeff_var": 18.4023159177328,
                "sum": 620861851.176612,
            },
            "blackman": {
                "n": 5625000,
                "null_cells": 0,
                "cells": 5625000,
                "min": 55.5969230376839,
                "max": 156.326357899112,
                "range": 100.729434861428,
                "mean": 110.375440150324,
                "mean_of_abs": 110.375440150324,
                "stddev": 20.3132139323513,
                "variance": 412.626660261469,
                "coeff_var": 18.4037444423197,
                "sum": 620861850.845649,
            },
        },
    }

    # TODO: replace by unified handing of maps
    to_remove = []

    def check_univar(self, test_case):
        """Checks multiple output against unviariate reference from dict"""
        for method in self.test_options[test_case]:
            self.assertRasterFitsUnivar(
                raster="{}_raster_{}".format(test_case, method),
                reference=self.test_options[test_case][method],
                precision=1e-5,
            )
            self.assertRasterFitsUnivar(
                raster="{}_raster_threaded_{}".format(test_case, method),
                reference=self.test_options[test_case][method],
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

    def test_finite_options(self):
        """Test output with finite kernels only."""
        test_case = "test_finite_options"
        outputs = [
            "{}_raster_{}".format(test_case, filter)
            for filter in self.test_options[test_case]
        ]
        outputs_threaded = [
            "{}_raster_threaded_{}".format(test_case, filter)
            for filter in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        self.to_remove.extend(outputs_threaded)

        for filter in self.test_options[test_case]:
            self.assertModule(
                "r.resamp.filter",
                input="elevation",
                output="{}_raster_{}".format(test_case, filter),
                filter=filter,
                radius=10,
            )
            self.assertModule(
                "r.resamp.filter",
                input="elevation",
                output="{}_raster_threaded_{}".format(test_case, filter),
                filter=filter,
                radius=10,
                nprocs=8,
            )
        self.check_univar(test_case)

    def test_infinite_box_options(self):
        """Test output with infinite kernels in conjunction
        with the box kernel."""
        test_case = "test_infinite_box_options"
        outputs = [
            "{}_raster_{}".format(test_case, filter)
            for filter in self.test_options[test_case]
        ]
        outputs_threaded = [
            "{}_raster_threaded_{}".format(test_case, filter)
            for filter in self.test_options[test_case]
        ]
        self.to_remove.extend(outputs)
        self.to_remove.extend(outputs_threaded)

        for filter in self.test_options[test_case]:
            self.assertModule(
                "r.resamp.filter",
                input="elevation",
                output="{}_raster_{}".format(test_case, filter),
                filter=["box", filter],
                radius=[10, 10],
            )
            self.assertModule(
                "r.resamp.filter",
                input="elevation",
                output="{}_raster_threaded_{}".format(test_case, filter),
                filter=["box", filter],
                radius=[10, 10],
                nprocs=8,
            )
        self.check_univar(test_case)


if __name__ == "__main__":
    test()
