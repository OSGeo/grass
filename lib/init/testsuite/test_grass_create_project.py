"""
TEST:      Test of grass -c error handling

AUTHOR(S): Akash Maity

PURPOSE:   Test that grass -c reports a clear error instead of a traceback

SPDX-FileCopyrightText: 2026 Akash Maity
SPDX-FileCopyrightText: GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
"""

import subprocess
import tempfile
import unittest
from pathlib import Path


class TestCreateProjectErrors(unittest.TestCase):
    """grass -c should print a clear error, not a traceback"""

    executable = "grass"

    def setUp(self):
        # A fresh empty folder for each test, deleted automatically afterwards
        self.tmp = tempfile.TemporaryDirectory()
        self.tmp_path = Path(self.tmp.name)

    def tearDown(self):
        self.tmp.cleanup()

    def test_missing_database_directory(self):
        """Missing parent directories give a clear error"""
        project = self.tmp_path / "missing" / "deeper" / "myproject"
        result = subprocess.run(
            [self.executable, "-c", "EPSG:4326", str(project), "-e"],
            capture_output=True,
            text=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertNotIn("Traceback", result.stderr)
        self.assertIn("Cannot create project", result.stderr)


if __name__ == "__main__":
    unittest.main()
