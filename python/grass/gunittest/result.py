"""Test result object"""

from __future__ import annotations

import unittest
from typing import TYPE_CHECKING, TextIO

if TYPE_CHECKING:
    import grass.gunittest.case


class TestResult(unittest.TestResult):
    """Holder for test result information.

    Test results are automatically managed by the TestCase and TestSuite
    classes, and do not need to be explicitly manipulated by writers of tests.

    Each instance holds the total number of tests run, and collections of
    failures and errors that occurred among those test runs. The collections
    contain tuples of (testcase, exceptioninfo), where exceptioninfo is the
    formatted traceback of the error that occurred.
    """

    start_time: float | None = None
    end_time: float | None = None
    time_taken: float | None = None

    # descriptions and verbosity unused
    # included for compatibility with unittest's TestResult
    # where are also unused, so perhaps we can remove them
    # stream set to None and not included in interface, it would not make sense
    def __init__(
        self,
        stream: TextIO | None = None,
        descriptions: bool | None = None,
        verbosity: int | None = None,
    ) -> None:
        super().__init__(stream=stream, descriptions=descriptions, verbosity=verbosity)
        self.successes: list[
            grass.gunittest.case.TestCase | unittest.case.TestCase
        ] = []
        self.start_time = None
        self.end_time = None
        self.time_taken = None

    def addSuccess(
        self, test: grass.gunittest.case.TestCase | unittest.case.TestCase
    ) -> None:
        super().addSuccess(test)
        self.successes.append(test)

    # TODO: better would be to pass start at the beginning
    # alternative is to leave counting time on class
    # TODO: alternatively, be more forgiving for non-unittest methods
    def setTimes(self, start_time: float, end_time: float, time_taken: float) -> None:
        """
        Store the start time, end time and time taken for a test.

        We expect all grass classes to have setTimes.

        :param start_time: The start time of the test, as returned by time.time()
        :param end_time: The end time of the test, as returned by time.time()
        :param time_taken: The time taken for the test, usually end_time-start_time
        """
        self.start_time = start_time
        self.end_time = end_time
        self.time_taken = time_taken
