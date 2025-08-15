"""Test result object"""

import unittest


class TestResult(unittest.TestResult):
    """Holder for test result information.

    Test results are automatically managed by the TestCase and TestSuite
    classes, and do not need to be explicitly manipulated by writers of tests.

    Each instance holds the total number of tests run, and collections of
    failures and errors that occurred among those test runs. The collections
    contain tuples of (testcase, exceptioninfo), where exceptioninfo is the
    formatted traceback of the error that occurred.
    """

    # descriptions and verbosity unused
    # included for compatibility with unittest's TestResult
    # where are also unused, so perhaps we can remove them
    # stream set to None and not included in interface, it would not make sense
    def __init__(self, stream=None, descriptions=None, verbosity=None):
        super().__init__(stream=stream, descriptions=descriptions, verbosity=verbosity)
        self.successes = []

    def addSuccess(self, test):
        super().addSuccess(test)
        self.successes.append(test)

    # TODO: better would be to pass start at the beginning
    # alternative is to leave counting time on class
    # TODO: document: we expect all grass classes to have setTimes
    # TODO: alternatively, be more forgiving for non-unittest methods
    def setTimes(self, start_time, end_time, time_taken):
        pass
        # TODO: implement this
