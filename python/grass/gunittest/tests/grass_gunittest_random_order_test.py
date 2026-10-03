"""Tests of running test files in random order with grass.gunittest"""

import argparse
import os
import re
from pathlib import Path

import pytest

from grass.gunittest.checkers import text_to_keyvalue
from grass.gunittest.invoker import GrassTestFilesInvoker
from grass.gunittest.loader import discover_modules, discovery_sort_key
from grass.gunittest.main import get_last_seed, seed_type
from grass.gunittest.reporters import to_web_path

# Directory names which sort differently as plain strings than as paths
# ("a-b" < "a/b" as strings, but "a" and its subdirectories come first
# in a directory walk).
TESTED_DIRS = ["a", "a/b", "a-b", "a.b", "b"]
TEST_FILES = ["test_a.py", "test_a_b.py", "test_b.py"]
NUM_TEST_FILES = len(TESTED_DIRS) * len(TEST_FILES)

# Each test file appends its path to a log file, so that the tests can
# check in which order the test files ran.
TEST_FILE_CODE = """
import os

with open(os.environ["ORDER_LOG"], "a", encoding="utf-8") as log:
    log.write(os.path.realpath(__file__) + "\\n")
"""


@pytest.fixture
def test_tree(tmp_path, monkeypatch):
    """Create a directory tree with testsuite directories and a location"""
    start_dir = tmp_path / "src"
    for tested_dir in TESTED_DIRS:
        testsuite = start_dir / tested_dir / "testsuite"
        testsuite.mkdir(parents=True)
        for name in TEST_FILES:
            (testsuite / name).write_text(TEST_FILE_CODE, encoding="utf-8")
    permanent = tmp_path / "grassdata" / "test" / "PERMANENT"
    permanent.mkdir(parents=True)
    (permanent / "DEFAULT_WIND").write_text("", encoding="utf-8")
    # The invoker places results of a test file into a directory derived
    # from the tested directory, so the start directory needs to be relative.
    monkeypatch.chdir(start_dir)
    monkeypatch.setenv("ORDER_LOG", str(tmp_path / "order.txt"))
    return tmp_path


def run_tests(tmp_path, results_name, random_seed=None):
    """Run all test files and return the order they ran in and the results"""
    log = Path(os.environ["ORDER_LOG"])
    log.unlink(missing_ok=True)
    results_dir = tmp_path / results_name
    invoker = GrassTestFilesInvoker(start_dir=".")
    reporter = invoker.run_in_location(
        gisdbase=str(tmp_path / "grassdata"),
        location="test",
        location_type="nc",
        results_dir=str(results_dir),
        exclude=[],
        random_seed=random_seed,
    )
    assert reporter.files_pass == NUM_TEST_FILES
    return log.read_text(encoding="utf-8").splitlines(), results_dir


def discover():
    """Return test files in the order of discovery"""
    modules = discover_modules(
        start_dir=".",
        skip_dirs=[],
        testsuite_dir="testsuite",
        grass_location="all",
        all_locations_value="all",
        universal_location_value="universal",
        import_modules=False,
        file_regexp=r".*\.py$",
    )
    assert len(modules) == NUM_TEST_FILES
    return modules


def real_paths(modules):
    return [os.path.realpath(module.abs_file_path) for module in modules]


def report_order(results_dir):
    """Return test files as listed in the reports and the key-value summary"""
    page = (results_dir / "testfiles.html").read_text(encoding="utf-8")
    html_order = re.findall(r'<td><a href="([^"]*)/([^"/]*)/index.html">', page)
    summary = text_to_keyvalue(
        (results_dir / "test_keyvalue_result.txt").read_text(encoding="utf-8"),
        sep="=",
    )
    keyvalue_order = list(zip(summary["tested_dirs"], summary["names"], strict=True))
    return html_order, keyvalue_order, summary


def test_discovery_sort_key_matches_discovery_order(test_tree):
    """Sorting by discovery_sort_key gives the order of discovery"""
    modules = discover()
    assert sorted(reversed(modules), key=discovery_sort_key) == modules


def test_default_order_is_discovery_order(test_tree):
    """Without a seed, test files run in the order of discovery"""
    order, unused_results_dir = run_tests(test_tree, "results")
    assert order == real_paths(discover())


def test_same_seed_gives_same_order(test_tree):
    """The same seed reproduces the order and a different seed changes it"""
    expected = real_paths(discover())
    first, unused_results_dir = run_tests(test_tree, "results_1", random_seed=1)
    second, unused_results_dir = run_tests(test_tree, "results_2", random_seed=1)
    other, unused_results_dir = run_tests(test_tree, "results_3", random_seed=2)
    assert first == second
    assert sorted(first) == sorted(expected)
    assert first != expected
    assert other != first


def test_reports_keep_discovery_order(test_tree):
    """Reports list test files in the order of discovery regardless of run order"""
    modules = discover()
    order, results_dir = run_tests(test_tree, "results", random_seed=1)
    assert order != real_paths(modules)
    html_order, keyvalue_order, summary = report_order(results_dir)
    assert html_order == [
        (to_web_path(module.tested_dir), module.name) for module in modules
    ]
    assert keyvalue_order == [(module.tested_dir, module.name) for module in modules]
    assert summary["random_seed"] == 1
    index_page = (results_dir / "index.html").read_text(encoding="utf-8")
    assert "random order with seed 1" in index_page


def test_no_seed_in_report_by_default(test_tree):
    """Reports mention a seed only when test files run in random order"""
    unused_order, results_dir = run_tests(test_tree, "results")
    unused_html_order, unused_keyvalue_order, summary = report_order(results_dir)
    assert "random_seed" not in summary
    index_page = (results_dir / "index.html").read_text(encoding="utf-8")
    assert "seed" not in index_page


def test_last_seed_is_read_from_report(test_tree):
    """The seed of the previous run is available from its report"""
    unused_order, results_dir = run_tests(test_tree, "results", random_seed=1234)
    assert get_last_seed(results_dir) == 1234


def test_no_last_seed_without_random_order(test_tree):
    """There is no last seed after a run in the usual order or no run"""
    unused_order, results_dir = run_tests(test_tree, "results")
    assert get_last_seed(results_dir) is None
    assert get_last_seed(test_tree / "does_not_exist") is None


def test_seed_type():
    """Seed is an integer or the special value last"""
    assert seed_type("42") == 42
    assert seed_type("last") == "last"
    with pytest.raises(argparse.ArgumentTypeError):
        seed_type("first")
