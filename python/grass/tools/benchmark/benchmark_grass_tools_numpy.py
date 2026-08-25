import time
import numpy as np


from grass.tools import Tools
from grass.benchmark import (
    num_cells_plot,
    benchmark_resolutions,
    load_results,
    save_results,
)


class TimeMeasurer:
    def __init__(self):
        self._time = None
        self._start = None

    @property
    def time(self):
        return self._time

    def start(self):
        self._start = time.perf_counter()

    def stop(self):
        self._time = time.perf_counter() - self._start


class PlainNumPyBenchmark(TimeMeasurer):
    def run(self):
        tools = Tools()
        region = tools.g_region(flags="p", format="json")
        a = np.full((region["rows"], region["cols"]), 1)
        b = np.full((region["rows"], region["cols"]), 1)

        self.start()
        c = 2 * np.sqrt(a + b) * np.sqrt(a) + np.sqrt(b) + a / 2
        self.stop()

        print(c.sum())
        print(c.size)

        del a
        del b
        del c


class PlainGRASSBenchmark(TimeMeasurer):
    def run(self):
        tools = Tools(overwrite=True)
        tools.r_mapcalc(expression="a = 1")
        tools.r_mapcalc(expression="b = 1")

        self.start()
        tools.r_mapcalc(expression="c = 2 * sqrt(a + b) * sqrt(a) * sqrt(b) + a / 2")
        self.stop()

        c_stats = tools.r_univar(map="c", format="json")
        print(c_stats["sum"])
        print(c_stats["cells"])


class NumPyGRASSBenchmark(TimeMeasurer):
    def run(self):
        tools = Tools()
        region = tools.g_region(flags="p", format="json")
        a = np.full((region["rows"], region["cols"]), 1)
        b = np.full((region["rows"], region["cols"]), 1)

        self.start()
        c = tools.r_mapcalc_simple(
            expression="2* sqrt(A + B) * sqrt(A) * sqrt(B) + A / 2",
            a=a,
            b=b,
            output=np.array,
        )
        self.stop()

        c_stats = tools.r_univar(map=c, format="json")
        print(c_stats["sum"])
        print(c_stats["cells"])

        del a
        del b
        del c


def main():
    resolutions = [5, 2, 1, 0.5]
    repeat = 10
    results = [
        benchmark_resolutions(
            module=PlainNumPyBenchmark(),
            label="NumPy",
            resolutions=resolutions,
            repeat=repeat,
        ),
        benchmark_resolutions(
            module=PlainGRASSBenchmark(),
            label="GRASS",
            resolutions=resolutions,
            repeat=repeat,
        ),
        benchmark_resolutions(
            module=NumPyGRASSBenchmark(),
            label="NumPy GRASS",
            resolutions=resolutions,
            repeat=repeat,
        ),
    ]
    print(results)
    results = load_results(save_results(results))
    print(results)
    plot_file = "test_res_plot.png"
    num_cells_plot(results.results, filename=plot_file)
    print(plot_file)


if __name__ == "__main__":
    main()
