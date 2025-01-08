from grass.gunittest.case import TestCase
from grass.gunittest.main import test


class TestError(TestCase):
    # pylint: disable=R0904

    def test_something(self):
        msg = "Error in test function"
        raise RuntimeError(msg)
        self.assertTrue(True)


class TestErrorSetUp(TestCase):
    # pylint: disable=R0904

    def setUp(self):
        msg = "Error in setUp"
        raise RuntimeError(msg)

    def test_something(self):
        self.assertTrue(True)


class TestErrorTearDown(TestCase):
    # pylint: disable=R0904

    def tearDown(self):
        msg = "Error in tearDown"
        raise RuntimeError(msg)

    def test_something(self):
        self.assertTrue(True)


class TestErrorClassSetUp(TestCase):
    # pylint: disable=R0904

    @classmethod
    def setUpClass(cls):
        msg = "Error in setUpClass"
        raise RuntimeError(msg)

    def test_something(self):
        self.assertTrue(True)


class TestErrorClassTearDown(TestCase):
    # pylint: disable=R0904

    @classmethod
    def tearDownClass(cls):
        msg = "Error in tearDownClass"
        raise RuntimeError(msg)

    def test_something(self):
        self.assertTrue(True)


if __name__ == "__main__":
    test()
