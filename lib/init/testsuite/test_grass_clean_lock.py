#!/usr/bin/env python3

"""
TEST:      Test of grass --tmp-mapset

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test that --tmp-mapset option of grass command works

COPYRIGHT: (C) 2020 Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import unittest
import os
import platform
import shutil
import subprocess
from pathlib import Path


# Note that unlike rest of GRASS GIS, here we are using unittest package
# directly. The grass.gunittest machinery for mapsets is not needed here.
# How this plays out together with the rest of testing framework is yet to be
# determined.


class TestCleanLock(unittest.TestCase):
    """Tests --no-clean --no-lock options of grass command"""

    # TODO: here we need a name of or path to the main GRASS GIS executable
    executable = "grass"
    # an arbitrary, but identifiable and fairly unique name
    location = "test_tmp_mapset_xy"
    mapset = os.path.join(location, "PERMANENT")

    def setUp(self):
        """Creates a location used in the tests"""
        subprocess.check_call([self.executable, "-c", "XY", self.location, "-e"])
        self.subdirs = os.listdir(self.location)

    def tearDown(self):
        """Deletes the location"""
        shutil.rmtree(self.location, ignore_errors=True)

    def test_command_runs(self):
        """Check that correct parameters are accepted"""
        return_code = subprocess.call(
            [self.executable, self.mapset, "--no-clean", "--exec", "g.proj", "-g"]
        )
        self.assertEqual(
            return_code,
            0,
            msg=(
                "Non-zero return code from {self.executable}"
                " with --no-clean".format(**locals())
            ),
        )

        return_code = subprocess.call(
            [self.executable, self.mapset, "--no-lock", "--exec", "g.proj", "-g"]
        )
        self.assertEqual(
            return_code,
            0,
            msg=(
                "Non-zero return code from {self.executable}"
                " with --no-lock".format(**locals())
            ),
        )

        return_code = subprocess.call(
            [
                self.executable,
                self.mapset,
                "--no-clean",
                "--no-lock",
                "--exec",
                "g.proj",
                "-g",
            ]
        )
        self.assertEqual(
            return_code,
            0,
            msg=(
                "Non-zero return code from {self.executable}"
                " with --no-clean and --no-lock".format(**locals())
            ),
        )

        return_code = subprocess.call([self.executable, self.mapset, "--clean-only"])
        self.assertEqual(
            return_code,
            0,
            msg=(
                "Non-zero return code from {self.executable}"
                " with --clean-only".format(**locals())
            ),
        )

    def test_clean_only_fails_with_exec(self):
        """Check that using --clean-only fails when --exec is provided"""
        return_code = subprocess.call(
            [self.executable, self.mapset, "--clean-only", "--exec", "g.proj", "-g"]
        )
        self.assertNotEqual(
            return_code,
            0,
            msg=("Zero retrun code from {self.executable}".format(**locals())),
        )

    def test_cleaning_fake_tmp_file(self):
        """Check that --no-clean does not delete existing temp files.

        Then it checks that the file is deleted without --no-clean.

        Assumes that clean_temp for cleaning tmp files in mapsets would delete
        file with name xxx.yyy when there is no process with PID xxx.
        It further assumes that there is no process with the PID we used,
        so clean_temp would delete the file.
        Finally, it assumes the naming and nesting of the tmp dir.
        """
        common_unix_max = 32768  # common max of unix PIDs
        fake_pid = common_unix_max + 1
        mapset_tmp_dir_name = Path(self.mapset) / ".tmp" / platform.node()
        mapset_tmp_dir_name.mkdir(parents=True, exist_ok=True)
        name = "{}.1".format(fake_pid)
        fake_tmp_file = mapset_tmp_dir_name / name
        fake_tmp_file.touch()
        subprocess.check_call(
            [self.executable, "--no-clean", self.mapset, "--exec", "g.proj", "-g"]
        )
        self.assertTrue(fake_tmp_file.exists(), msg="File should still exist")
        subprocess.check_call([self.executable, self.mapset, "--exec", "g.proj", "-g"])
        self.assertFalse(fake_tmp_file.exists(), msg="File should have been deleted")

    def test_cleaning_g_tempfile(self):
        """Check that --no-clean does not delete existing temp files.

        Then it checks that the file is deleted without --no-clean.

        This makes no assumptions about inner workings of tmp file cleaning,
        but it relies on g.tempfile to work correctly and that there no
        other files in the tmp dir.
        It still needs to assume that there is no process with the PID we used,
        so clean_temp would delete the file.
        """
        common_unix_max = 32768  # common max of unix PIDs
        fake_pid = common_unix_max + 1
        mapset_tmp_dir_name = Path(self.mapset) / ".tmp" / platform.node()
        # we need --no-clean even here otherwise we delete the file at exit
        subprocess.check_call(
            [
                self.executable,
                self.mapset,
                "--no-clean",
                "--exec",
                "g.tempfile",
                "pid={}".format(fake_pid),
            ]
        )
        # another process
        subprocess.check_call(
            [self.executable, "--no-clean", self.mapset, "--exec", "g.proj", "-g"]
        )
        self.assertTrue(
            os.listdir(str(mapset_tmp_dir_name)), msg="File should still exist"
        )
        subprocess.check_call([self.executable, self.mapset, "--exec", "g.proj", "-g"])
        self.assertFalse(
            os.listdir(str(mapset_tmp_dir_name)), msg="File should have been deleted"
        )


if __name__ == "__main__":
    unittest.main()
