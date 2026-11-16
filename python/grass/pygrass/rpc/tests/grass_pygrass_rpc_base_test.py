# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test the RPC server base class"""

import threading
import time

import pytest

from grass.exceptions import FatalError
from grass.pygrass.rpc import base


def test_no_restart_at_exit(monkeypatch):
    """A server terminated at exit is not restarted and the check raises"""
    provider = base.RPCServerBase()
    try:
        monkeypatch.setattr(base, "_mp_is_exiting", lambda: True)
        server = provider.server
        server.terminate()
        server.join()
        with pytest.raises(FatalError, match="terminated at exit"):
            provider.check_server()
        assert provider.server is server
        assert not provider.is_server_alive()
    finally:
        provider.stop()


def wait_for(condition, timeout=5):
    """Return True once condition() is true, False after timeout seconds"""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if condition():
            return True
        time.sleep(0.02)
    return condition()


@pytest.fixture
def provider():
    """A server with a short check interval to keep the tests quick"""
    provider = base.RPCServerBase()
    provider.check_interval = 0.05
    try:
        yield provider
    finally:
        provider.stop()


def test_crashed_idle_server_is_restarted(provider):
    """The checker thread replaces a server which died while nobody waits"""
    crashed = provider.server
    crashed.kill()
    # The replacement is assigned before it is started, and a process which is
    # not started yet is not alive. Starting takes long with the spawn start
    # method, so wait for both.
    assert wait_for(
        lambda: provider.server is not crashed and provider.is_server_alive()
    )


def test_client_waiting_for_reply_gets_error_when_server_crashes(provider):
    """A client blocked in safe_receive() is woken up when the server dies

    The parent keeps its own copy of the server end of the pipe, so the dead
    server alone gives no end of file. The checker thread closing the pipe is
    what wakes the client.
    """
    raised = []

    def wait_for_reply():
        try:
            provider.safe_receive("a reply which never comes")
        except FatalError as error:
            raised.append(error)

    # The server ignores this request and does not send a reply.
    provider.client_conn.send([2])
    client = threading.Thread(target=wait_for_reply, daemon=True)
    client.start()
    # Give the client time to start waiting, then crash the server.
    time.sleep(0.2)
    provider.server.kill()
    client.join(timeout=10)
    assert not client.is_alive(), "The client is still blocked"
    assert len(raised) == 1


def test_stopped_server_is_not_restarted(provider):
    """A server which was stopped on purpose stays stopped"""
    provider.stop()
    stopped = provider.server
    time.sleep(5 * provider.check_interval)
    assert provider.server is stopped
    assert not provider.is_server_alive()
    assert not provider.is_check_thread_alive()


def test_stop_twice(provider):
    """Stopping an already stopped server is harmless"""
    provider.stop()
    provider.stop()
    assert not provider.is_server_alive()


def test_starting_checker_thread_again_replaces_the_thread(provider):
    """Only one checker thread stays alive when it is started repeatedly"""
    first = provider.checkThread
    provider.start_checker_thread()
    second = provider.checkThread
    assert second is not first
    assert not first.is_alive()
    assert second.is_alive()
    provider.stop()
    assert not second.is_alive()


def test_stop_does_not_wait_for_the_check_interval():
    """Stopping returns at once however long the check interval is"""

    class SlowChecker(base.RPCServerBase):
        check_interval = 60

    provider = SlowChecker()
    started = time.monotonic()
    provider.stop()
    # The limit is far above a normal stop and far below the interval, so only
    # a stop which waits for the interval can fail.
    assert time.monotonic() - started < 10
    assert not provider.is_check_thread_alive()
