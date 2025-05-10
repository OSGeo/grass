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
import shutil
import subprocess


# Note that unlike rest of GRASS GIS, here we are using unittest package
# directly. The grass.gunittest machinery for mapsets is not needed here.
# How this plays out together with the rest of testing framework is yet to be
# determined.


class TestTmpMapset(unittest.TestCase):
    """Tests --tmp-mapset option of grass command"""

    # TODO: here we need a name of or path to the main GRASS GIS executable
    # TODO: support OSGeo4W executable with:
    # executable = "grass" if os.name != "nt" else "grass85.bat"
    executable = "grass" if os.name != "nt" else "grass.bat"
    # an arbitrary, but identifiable and fairly unique name
    location = "test_tmp_mapset_xy"

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
            [self.executable, "--tmp-mapset", self.location, "--exec", "g.proj", "-g"]
        )
        self.assertEqual(
            return_code,
            0,
            msg=(
                "Non-zero return code from {self.executable}"
                " when creating mapset".format(**locals())
            ),
        )

    def test_command_fails_without_location(self):
        """Check that the command fails with a nonexistent location"""
        return_code = subprocess.call(
            [
                self.executable,
                "--tmp-mapset",
                "does_not_exist",
                "--exec",
                "g.proj",
                "-g",
            ]
        )
        self.assertNotEqual(
            return_code,
            0,
            msg=(
                "Zero return code from {self.executable},"
                " but the location directory does not exist".format(**locals())
            ),
        )

    def test_mapset_metadata_correct(self):
        """Check that metadata is readable and have expected value (XY CRS)"""
        output = subprocess.check_output(
            [self.executable, "--tmp-mapset", self.location, "--exec", "g.proj", "-g"]
        )
        self.assertEqual(
            output.strip(),
            "name=xy_location_unprojected".encode("ascii"),
            msg="Mapset metadata are not what was expected, but: {output}".format(
                **locals()
            ),
        )

    def test_mapset_deleted(self):
        """Check that mapset is deleted at the end of execution"""
        subprocess.check_call(
            [self.executable, "--tmp-mapset", self.location, "--exec", "g.proj", "-p"]
        )
        for directory in os.listdir(self.location):
            self.assertTrue(
                directory in self.subdirs,
                msg="Directory {directory} should have been deleted".format(**locals()),
            )


if __name__ == "__main__":
    unittest.main()
