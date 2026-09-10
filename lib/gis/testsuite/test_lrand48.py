"""Test of gis library lrand48 PRNG thread-safety

@author Maris Nartiss

@copyright 2025 by the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

import threading

from grass.gunittest.case import TestCase
from grass.gunittest.main import test
from grass.lib.gis import G_lrand48, G_srand48


class Lrand48ThreadSafetyTestCase(TestCase):
    """Test case for lrand48 thread-safety and reproducibility."""

    def test_thread_safety_and_reproducibility(self):
        """Verify that multi-threaded execution produces the same set of
        random numbers as single-threaded execution."""

        seed = 1337
        num_values = 10000
        num_threads = 4
        values_per_thread = num_values // num_threads

        list_single = []
        G_srand48(seed)
        for _ in range(num_values):
            list_single.append(G_lrand48())

        list_multi_raw = []
        lock = threading.Lock()

        def worker():
            """Calls G_lrand48 and appends the result to a shared list."""
            local_results = []
            for _ in range(values_per_thread):
                local_results.append(G_lrand48())

            # The lock protects only the Python result list, not the C
            # generator under test (list.extend is atomic under the GIL,
            # but that is a CPython implementation detail).
            with lock:
                list_multi_raw.extend(local_results)

        # Reset the seed to ensure the sequence starts from the beginning.
        G_srand48(seed)

        threads = []
        for _ in range(num_threads):
            thread = threading.Thread(target=worker)
            threads.append(thread)
            thread.start()

        for thread in threads:
            thread.join()

        # The generator serializes state updates, so the threads together
        # must consume exactly the single-threaded sequence; only the
        # distribution of values between threads may differ. Sorting both
        # lists removes the scheduling-dependent order before comparison.
        # A set-based comparison would not work, because a correct
        # sequence can contain legitimate duplicates.
        self.assertListEqual(
            sorted(list_single),
            sorted(list_multi_raw),
            "The set of generated numbers differs between "
            "single-threaded and multi-threaded runs.",
        )


if __name__ == "__main__":
    test()
