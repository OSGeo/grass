# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of G_nprocs_from_option() and G_resolve_nprocs()"""

import subprocess
import sys

import pytest

from grass.tools import ToolError

# Prints G_nprocs_from_option() and G_set_omp_num_threads() for the answer
# in argv[1] (empty: an option without an answer).
SCRIPT = """
import sys
from ctypes import byref

from grass.lib.gis import G_nprocs_from_option, G_set_omp_num_threads, Option, String

option = Option()
option.key = String(b"nprocs")
option.answer = String(sys.argv[1].encode() or None)
print(G_nprocs_from_option(byref(option)), G_set_omp_num_threads(byref(option)))
"""


def nprocs_from_option(session, answer, omp_num_threads=None):
    """Exit code, the two numbers SCRIPT prints, and stderr for an answer

    OpenMP reads OMP_NUM_THREADS when it is loaded, so the variable is
    set (None: unset) for a new process.
    """
    env = session.env.copy()
    env.pop("OMP_NUM_THREADS", None)
    if omp_num_threads is not None:
        env["OMP_NUM_THREADS"] = omp_num_threads
    result = subprocess.run(
        [sys.executable, "-c", SCRIPT, answer],
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )
    numbers = [int(number) for number in result.stdout.split()]
    return result.returncode, numbers, result.stderr


# Answers which the parser lets through for an integer option
PASS_THE_PARSER = ["2x", "50%"]


@pytest.mark.parametrize("answer", ["99999999999", "", " ", *PASS_THE_PARSER])
def test_not_an_int(xy_session_for_module, answer):
    """An answer which is not an int is an error naming the option"""
    returncode, _, stderr = nprocs_from_option(xy_session_for_module, answer)
    assert returncode != 0
    assert f"Invalid value <{answer}> for option <nprocs>" in stderr


@pytest.mark.parametrize("answer", PASS_THE_PARSER)
def test_tool_refuses_not_an_int(tools, answer):
    """A tool refuses the answers of test_not_an_int which pass the parser"""
    with pytest.raises(ToolError) as error:
        tools.r_univar(map="doesnotexist", flags="g", nprocs=answer)
    assert f"Invalid value <{answer}> for option <nprocs>" in error.value.stderr


@pytest.mark.parametrize(("answer", "expected"), [(" 3 \n", 3), ("100", 100)])
def test_positive_as_given(xy_session_for_module, answer, expected):
    """A positive number is used as given, with white space around it"""
    returncode, numbers, stderr = nprocs_from_option(xy_session_for_module, answer)
    assert returncode == 0, stderr
    assert numbers[0] == expected


def test_omp_num_threads_sets_only_the_default(xy_session_for_module):
    """OMP_NUM_THREADS changes what 0 gives, but not what negative values give,
    which give at least one thread
    """
    session = xy_session_for_module
    returncode, numbers, stderr = nprocs_from_option(session, "2")
    assert returncode == 0, stderr
    if numbers[1] != 2:
        pytest.skip("GRASS is built without OpenMP")
    assert nprocs_from_option(session, "0", omp_num_threads="6")[1] == [6, 6]
    default = nprocs_from_option(session, "-1")[1]
    assert default
    assert nprocs_from_option(session, "-1", omp_num_threads="1000")[1] == default
    assert nprocs_from_option(session, "-100000")[1] == [1, 1]
