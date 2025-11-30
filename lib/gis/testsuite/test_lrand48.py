"""Test of gis library lrand48 PRNG thread-safety

@author Maris Nartiss
@author Gemini

@copyright 2025 by the GRASS Development Team

@license This program is free software under the GNU General Public License (>=v2).
Read the file COPYING that comes with GRASS
for details
"""

import ctypes
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

        self.assertEqual(
            num_values % num_threads,
            0,
            "Total number of values must be divisible by the number of threads.",
        )

        # --- Define ctypes function signatures ---
        G_srand48.argtypes = [ctypes.c_long]
        G_srand48.restype = None

        G_lrand48.argtypes = []
        G_lrand48.restype = ctypes.c_long

        # --- 1. Single-threaded execution ---
        list_single = []
        G_srand48(seed)
        for _ in range(num_values):
            list_single.append(G_lrand48())

        # --- 2. Multi-threaded execution ---
        list_multi_raw = []
        lock = threading.Lock()

        def worker():
            """Calls G_lrand48 and appends the result to a shared list."""
            local_results = []
            for _ in range(values_per_thread):
                # G_lrand48() itself is protected by a C-level mutex
                local_results.append(G_lrand48())

            # Use a Python-level lock to safely extend the shared list
            with lock:
                list_multi_raw.extend(local_results)

        # Reset the seed to ensure the sequence starts from the beginning
        G_srand48(seed)

        threads = []
        for _ in range(num_threads):
            thread = threading.Thread(target=worker)
            threads.append(thread)
            thread.start()

        for thread in threads:
            thread.join()

        # --- 3. Verification ---
        self.assertEqual(
            len(list_single),
            len(list_multi_raw),
            "Single-threaded and multi-threaded runs produced a different number of values.",
        )

        # Check for duplicates in the multi-threaded list. The presence of duplicates
        # would indicate that the C-level mutex failed and multiple threads
        # received the same random number.
        self.assertEqual(
            len(list_multi_raw),
            len(set(list_multi_raw)),
            "Duplicate values found in multi-threaded run, indicating a race condition.",
        )

        # The sorted lists of numbers must be identical.
        # This confirms that although threads ran in parallel, the C-level mutex
        # correctly serialized access to the PRNG, yielding the exact same
        # block of numbers.
        self.assertListEqual(
            sorted(list_single),
            sorted(list_multi_raw),
            "The set of generated numbers differs between single-threaded and multi-threaded runs.",
        )


if __name__ == "__main__":
    test()
