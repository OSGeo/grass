#!/usr/bin/env python3

"""
TEST:      Test of grass --tmp-subproject

AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>

PURPOSE:   Test that --tmp-subproject option of grass command works

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
# directly. The grass.gunittest machinery for subprojects is not needed here.
# How this plays out together with the rest of testing framework is yet to be
# determined.


class TestTmpSubproject(unittest.TestCase):
    """Tests --tmp-subproject option of grass command"""

    # TODO: here we need a name of or path to the main GRASS GIS executable
    executable = "grass"
    # an arbitrary, but identifiable and fairly unique name
    project = "test_tmp_subproject_xy"

    def setUp(self):
        """Creates a project used in the tests"""
        subprocess.check_call([self.executable, "-c", "XY", self.project, "-e"])
        self.subdirs = os.listdir(self.project)

    def tearDown(self):
        """Deletes the project"""
        shutil.rmtree(self.project, ignore_errors=True)

    def test_command_runs(self):
        """Check that correct parameters are accepted"""
        return_code = subprocess.call(
            [self.executable, "--tmp-subproject", self.project, "--exec", "g.proj", "-g"]
        )
        self.assertEqual(
            return_code,
            0,
            msg=(
                "Non-zero return code from {self.executable}"
                " when creating subproject".format(**locals())
            ),
        )

    def test_command_fails_without_project(self):
        """Check that the command fails with a nonexistent project"""
        return_code = subprocess.call(
            [
                self.executable,
                "--tmp-subproject",
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
                " but the project directory does not exist".format(**locals())
            ),
        )

    def test_subproject_metadata_correct(self):
        """Check that metadata is readable and have expected value (XY CRS)"""
        output = subprocess.check_output(
            [self.executable, "--tmp-subproject", self.project, "--exec", "g.proj", "-g"]
        )
        self.assertEqual(
            output.strip(),
            "name=xy_project_unprojected".encode("ascii"),
            msg="Subproject metadata are not what was expected, but: {output}".format(
                **locals()
            ),
        )

    def test_subproject_deleted(self):
        """Check that subproject is deleted at the end of execution"""
        subprocess.check_call(
            [self.executable, "--tmp-subproject", self.project, "--exec", "g.proj", "-p"]
        )
        for directory in os.listdir(self.project):
            self.assertTrue(
                directory in self.subdirs,
                msg="Directory {directory} should have been deleted".format(**locals()),
            )


if __name__ == "__main__":
    unittest.main()
