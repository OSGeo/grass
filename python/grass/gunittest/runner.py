"""
Testing framework module for running tests in Python unittest fashion

Copyright (C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS GIS
for details.

:authors: Vaclav Petras

File content taken from Python's  ``unittest.runner``, it will be used as
a template. It is not expected that something will left.
"""

import sys
import time

import unittest

__unittest = True


class _WritelnDecorator:
    """Used to decorate file-like objects with a handy 'writeln' method"""

    def __init__(self, stream):
        self.stream = stream

    def __getattr__(self, attr: str):
        if attr in {"stream", "__getstate__"}:
            raise AttributeError(attr)
        return getattr(self.stream, attr)

    def writeln(self, arg=None):
        if arg:
            self.write(arg)
        self.write("\n")  # text-mode streams translate to \r\n if needed


class TestResult(unittest.TestResult):
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


class TextTestResult(TestResult):
    """A test result class that can print formatted text results to a stream.

    Used by TextTestRunner.
    """

    separator1 = "=" * 70
    separator2 = "-" * 70

    def __init__(self, stream, descriptions, verbosity):
        super().__init__(stream=stream, descriptions=descriptions, verbosity=verbosity)
        self.stream = _WritelnDecorator(stream)
        self.showAll = verbosity > 1
        self.dots = verbosity == 1
        self.descriptions = descriptions

        self.start_time = None
        self.end_time = None
        self.time_taken = None

    def getDescription(self, test):
        doc_first_line = test.shortDescription()
        if self.descriptions and doc_first_line:
            return "\n".join((str(test), doc_first_line))
        return str(test)

    def startTest(self, test):
        super().startTest(test)
        if self.showAll:
            self.stream.write(self.getDescription(test))
            self.stream.write(" ... ")
            self.stream.flush()

    def addSuccess(self, test):
        super().addSuccess(test)
        if self.showAll:
            self.stream.writeln("ok")
        elif self.dots:
            self.stream.write(".")
            self.stream.flush()

    def addError(self, test, err):
        super().addError(test, err)
        if self.showAll:
            self.stream.writeln("ERROR")
        elif self.dots:
            self.stream.write("E")
            self.stream.flush()

    def addFailure(self, test, err):
        super().addFailure(test, err)
        if self.showAll:
            self.stream.writeln("FAIL")
        elif self.dots:
            self.stream.write("F")
            self.stream.flush()

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        if self.showAll:
            self.stream.writeln("skipped {0!r}".format(reason))
        elif self.dots:
            self.stream.write("s")
            self.stream.flush()

    def addExpectedFailure(self, test, err):
        super().addExpectedFailure(test, err)
        if self.showAll:
            self.stream.writeln("expected failure")
        elif self.dots:
            self.stream.write("x")
            self.stream.flush()

    def addUnexpectedSuccess(self, test):
        super().addUnexpectedSuccess(test)
        if self.showAll:
            self.stream.writeln("unexpected success")
        elif self.dots:
            self.stream.write("u")
            self.stream.flush()

    def printErrors(self):
        if self.dots or self.showAll:
            self.stream.writeln()
        self.printErrorList("ERROR", self.errors)
        self.printErrorList("FAIL", self.failures)

    def printErrorList(self, flavour, errors):
        for test, err in errors:
            self.stream.writeln(self.separator1)
            self.stream.writeln("%s: %s" % (flavour, self.getDescription(test)))
            self.stream.writeln(self.separator2)
            self.stream.writeln("%s" % err)

    def setTimes(self, start_time, end_time, time_taken):
        self.start_time = start_time
        self.end_time = end_time
        self.time_taken = time_taken

    def stopTestRun(self):
        super().stopTestRun()
        self.printErrors()
        self.stream.writeln(self.separator2)
        run = self.testsRun
        self.stream.write("Ran %d test%s" % (run, (run != 1 and "s") or ""))
        if self.time_taken:
            self.stream.write(" in %.3fs" % (self.time_taken))
        self.stream.writeln()

        expectedFails = unexpectedSuccesses = skipped = 0
        results = map(
            len, (self.expectedFailures, self.unexpectedSuccesses, self.skipped)
        )
        expectedFails, unexpectedSuccesses, skipped = results

        infos = []
        if not self.wasSuccessful():
            self.stream.write("FAILED")
            failed, errored = map(len, (self.failures, self.errors))
            if failed:
                infos.append("failures=%d" % failed)
            if errored:
                infos.append("errors=%d" % errored)
        else:
            self.stream.write("OK")
        if skipped:
            infos.append("skipped=%d" % skipped)
        if expectedFails:
            infos.append("expected_failures=%d" % expectedFails)
        if unexpectedSuccesses:
            infos.append("unexpected_successes=%d" % unexpectedSuccesses)
        if infos:
            self.stream.writeln(" (%s)" % (", ".join(infos),))
        else:
            self.stream.write("\n")


class KeyValueTestResult(TestResult):
    """A test result class that can print formatted text results to a stream.

    Used by TextTestRunner.
    """

    separator1 = "=" * 70
    separator2 = "-" * 70

    def __init__(self, stream, test_type=None):
        super().__init__(stream=stream, descriptions=None, verbosity=None)
        self._stream = _WritelnDecorator(stream)

        self.start_time = None
        self.end_time = None
        self.time_taken = None

        if test_type:
            self.test_type = test_type
        else:
            self.test_type = "not-specified"

        self._grass_modules = []
        self._supplementary_files = []

    def setTimes(self, start_time, end_time, time_taken):
        self.start_time = start_time
        self.end_time = end_time
        self.time_taken = time_taken

    def stopTest(self, test):
        super().stopTest(test)
        if hasattr(test, "grass_modules"):
            self._grass_modules.extend(test.grass_modules)
        if hasattr(test, "supplementary_files"):
            self._supplementary_files.extend(test.supplementary_files)

    def stopTestRun(self):
        super().stopTestRun()
        infos = []

        run = self.testsRun
        # TODO: name should be included by test file caller
        # from inspect import getsourcefile
        # from os.path import abspath
        # abspath(getsourcefile(lambda _: None))
        # not writing name is a good option
        # infos.append("name=%s" % 'unknown')

        infos.append("time=%.3fs" % (self.time_taken))
        #            'date={rundate}\n'
        #            'date={runtime}\n'
        #            'date={start_datetime}\n'
        #            'date={end_datetime}\n'

        failed, errored = map(len, (self.failures, self.errors))
        succeeded = len(self.successes)
        results = map(
            len, (self.expectedFailures, self.unexpectedSuccesses, self.skipped)
        )
        expectedFails, unexpectedSuccesses, skipped = results

        status = "succeeded" if self.wasSuccessful() else "failed"
        infos.append("status=%s" % status)

        # if only errors occur, tests are not counted properly
        # in unittest, so reconstruct their count here
        # (using general equation, although now errors would be enough)
        # alternative is to behave as failed file, i.e. do not
        # write test details and just write status=failed
        if not run:
            run = errored + failed + succeeded
        infos.extend(
            (
                "total=%d" % (run),
                "failures=%d" % failed,
                "errors=%d" % errored,
                "successes=%d" % succeeded,
                "skipped=%d" % skipped,
                # TODO: document this: if not supported by view,
                # expected_failures should be counted as failures and vice versa
                # or both add to skipped as unclear?
                "expected_failures=%d" % expectedFails,
                "unexpected_successes=%d" % unexpectedSuccesses,
                # TODO: include each module just once? list good and bad modules?
                "tested_modules=%s" % ",".join(self._grass_modules),
                "supplementary_files=%s" % ",".join(self._supplementary_files),
                # module, modules?, c, c++?, python
                # TODO: include also type modules?
                # TODO: include also C++ code?
                # TODO: distinguish C and Python modules?
                "test_type=%s" % (self.test_type),
            )
        )

        self._stream.write("\n".join(infos))
        self._stream.write("\n")
        self._stream.flush()


class MultiTestResult(TestResult):
    # descriptions and verbosity unused
    # included for compatibility with unittest's TestResult
    # where are also unused, so perhaps we can remove them
    # stream set to None and not included in interface, it would not make sense
    def __init__(self, results, forgiving=False, descriptions=None, verbosity=None):
        super().__init__(descriptions=descriptions, verbosity=verbosity, stream=None)
        self._results = results
        self._forgiving = forgiving

    def startTest(self, test):
        super().startTest(test)
        for result in self._results:
            try:
                result.startTest(test)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def stopTest(self, test):
        """Called when the given test has been run"""
        super().stopTest(test)
        for result in self._results:
            try:
                result.stopTest(test)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def addSuccess(self, test):
        super().addSuccess(test)
        for result in self._results:
            try:
                result.addSuccess(test)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def addError(self, test, err):
        super().addError(test, err)
        for result in self._results:
            try:
                result.addError(test, err)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def addFailure(self, test, err):
        super().addFailure(test, err)
        for result in self._results:
            try:
                result.addFailure(test, err)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        for result in self._results:
            try:
                result.addSkip(test, reason)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def addExpectedFailure(self, test, err):
        super().addExpectedFailure(test, err)
        for result in self._results:
            try:
                result.addExpectedFailure(test, err)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def addUnexpectedSuccess(self, test):
        super().addUnexpectedSuccess(test)
        for result in self._results:
            try:
                result.addUnexpectedSuccess(test)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def printErrors(self):
        """Called by TestRunner after test run"""
        super().printErrors()
        for result in self._results:
            try:
                result.printErrors()
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def startTestRun(self):
        """Called once before any tests are executed.

        See startTest for a method called before each test.
        """
        super().startTestRun()
        for result in self._results:
            try:
                result.startTestRun()
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    def stopTestRun(self):
        """Called once after all tests are executed.

        See stopTest for a method called after each test.
        """
        super().stopTestRun()
        for result in self._results:
            try:
                result.stopTestRun()
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise

    # TODO: better would be to pass start at the beginning
    # alternative is to leave counting time on class
    # TODO: document: we expect all grass classes to have setTimes
    # TODO: alternatively, be more forgiving for non-unittest methods
    def setTimes(self, start_time, end_time, time_taken):
        for result in self._results:
            try:
                result.setTimes(start_time, end_time, time_taken)
            except AttributeError:
                if self._forgiving:
                    pass
                else:
                    raise


class GrassTestRunner:
    def __init__(
        self,
        stream=sys.stderr,
        descriptions=True,
        verbosity=1,
        failfast=False,
        buffer=False,
        result=None,
    ):
        self.stream = _WritelnDecorator(stream)
        self.descriptions = descriptions
        self.verbosity = verbosity
        self.failfast = failfast
        self.buffer = buffer
        self._result = result

    def run(self, test):
        """Run the given test case or test suite."""
        result = self._result
        unittest.registerResult(result)
        result.failfast = self.failfast
        result.buffer = self.buffer
        startTime = time.time()
        startTestRun = getattr(result, "startTestRun", None)
        if startTestRun is not None:
            startTestRun()
        try:
            test(result)
        finally:
            stopTime = time.time()
            timeTaken = stopTime - startTime
            setTimes = getattr(result, "setTimes", None)
            if setTimes is not None:
                setTimes(startTime, stopTime, timeTaken)
            stopTestRun = getattr(result, "stopTestRun", None)
            if stopTestRun is not None:
                stopTestRun()

        return result
