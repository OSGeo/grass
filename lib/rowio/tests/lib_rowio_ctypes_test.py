"""Tests for the row cache (rowio) ctypes bindings

ROWIO caches rows in memory on top of a caller-supplied pair of read/write
callbacks (getrow/putrow); it never touches a file or GRASS session itself,
so plain Python functions backed by an in-memory dict stand in for the
backing store here, and no GRASS session is needed.
"""

from contextlib import contextmanager
from ctypes import CFUNCTYPE, byref, c_int, c_void_p, memmove, string_at

from grass.lib import rowio as librowio

ROW_LENGTH = 4  # bytes per row; arbitrary, just has to match get/put calls

GETROW = CFUNCTYPE(c_int, c_int, c_void_p, c_int, c_int)
PUTROW = CFUNCTYPE(c_int, c_int, c_void_p, c_int, c_int)


@contextmanager
def rowio(nrows, backing_store):
    """A ROWIO caching `nrows` rows in front of `backing_store`

    Yields (R, calls), where calls records each ("get" | "put", row) that
    reached the backing store, in order. Rows not yet in backing_store read
    as b"\\x00" * ROW_LENGTH.
    """
    calls = []

    @GETROW
    def getrow(fd, buf, row, length):
        calls.append(("get", row))
        memmove(buf, backing_store.get(row, b"\x00" * length), length)
        return 1

    @PUTROW
    def putrow(fd, buf, row, length):
        calls.append(("put", row))
        backing_store[row] = string_at(buf, length)
        return 1

    r = librowio.ROWIO()
    assert librowio.Rowio_setup(byref(r), 0, nrows, ROW_LENGTH, getrow, putrow) == 1
    try:
        yield r, calls
    finally:
        librowio.Rowio_release(byref(r))


def get(r, row):
    """Call Rowio_get() and return the row's bytes"""
    buf = librowio.Rowio_get(byref(r), row)
    return string_at(buf, ROW_LENGTH)


def test_get_reads_through_the_backing_store_once() -> None:
    with rowio(2, {0: b"aaaa"}) as (r, calls):
        assert get(r, 0) == b"aaaa"
        assert get(r, 0) == b"aaaa"
        assert calls == [("get", 0)]


def test_put_on_a_cached_row_defers_the_write() -> None:
    """Rowio_put() on a row already in the cache just marks it dirty; the
    backing store is only written on eviction or Rowio_flush()"""
    with rowio(2, {0: b"aaaa"}) as (r, calls):
        get(r, 0)
        calls.clear()

        assert librowio.Rowio_put(byref(r), b"ZZZZ", 0) == 1
        assert calls == []
        assert get(r, 0) == b"ZZZZ"
        assert calls == []


def test_put_on_an_uncached_row_writes_immediately() -> None:
    with rowio(2, {}) as (r, calls):
        assert librowio.Rowio_put(byref(r), b"YYYY", 5) == 1
        assert calls == [("put", 5)]


def test_flush_writes_pending_dirty_rows() -> None:
    with rowio(2, {1: b"bbbb"}) as (r, calls):
        get(r, 1)
        librowio.Rowio_put(byref(r), b"VVVV", 1)
        calls.clear()

        librowio.Rowio_flush(byref(r))

        assert calls == [("put", 1)]


def test_lru_evicts_the_least_recently_used_row() -> None:
    backing = {i: bytes([i]) * ROW_LENGTH for i in range(4)}
    with rowio(2, backing) as (r, calls):
        get(r, 0)
        get(r, 1)
        get(r, 0)  # row 0 is now more recently used than row 1
        calls.clear()

        get(r, 2)  # cache is full: evicts row 1, not row 0

        assert calls == [("get", 2)]
        assert get(r, 0) == bytes([0]) * ROW_LENGTH  # still cached, no reread
        assert calls == [("get", 2)]


def test_lru_eviction_writes_back_a_dirty_row_first() -> None:
    backing = {i: bytes([i]) * ROW_LENGTH for i in range(4)}
    with rowio(2, backing) as (r, calls):
        get(r, 0)
        get(r, 1)
        get(r, 0)  # row 0 is now more recently used than row 1
        librowio.Rowio_put(byref(r), b"XXXX", 1)  # dirties row 1, still cached
        calls.clear()

        get(r, 2)  # evicts row 1 (the LRU one): must write it back first

        assert calls == [("put", 1), ("get", 2)]
        assert backing[1] == b"XXXX"


def test_forget_discards_unwritten_changes() -> None:
    """Rowio_forget() drops a row from the cache without flushing it, so a
    row put() but never flushed is silently lost"""
    backing = {0: b"aaaa"}
    with rowio(2, backing) as (r, calls):
        get(r, 0)
        librowio.Rowio_put(byref(r), b"WWWW", 0)

        librowio.Rowio_forget(byref(r), 0)

        assert calls == [("get", 0)]  # no put(0) was ever recorded
        assert backing[0] == b"aaaa"  # the backing store still has the old data


def test_forget_of_the_last_accessed_row_leaves_a_stale_fast_path() -> None:
    """Rowio_get() for the same row twice in a row skips the cache scan
    entirely and returns the buffer straight away (the "cur" shortcut).
    Rowio_forget() does not clear this shortcut, so getting the very row
    just forgotten returns the old buffer contents with no getrow() call
    at all, rather than reloading it. Accessing a different row first
    clears the shortcut and restores the normal, correct behavior.
    """
    backing = {0: b"aaaa", 1: b"bbbb"}

    with rowio(2, dict(backing)) as (r, calls):
        get(r, 0)
        librowio.Rowio_forget(byref(r), 0)
        calls.clear()

        assert get(r, 0) == b"aaaa"
        assert calls == []  # no getrow() call: the stale shortcut served it

    with rowio(2, dict(backing)) as (r, calls):
        get(r, 0)
        librowio.Rowio_forget(byref(r), 0)
        get(r, 1)  # touching a different row clears the shortcut
        calls.clear()

        assert get(r, 0) == b"aaaa"
        assert calls == [("get", 0)]  # correctly reloaded this time


def test_fileno_returns_the_configured_descriptor() -> None:
    with rowio(2, {}) as (r, _calls):
        assert librowio.Rowio_fileno(byref(r)) == 0
