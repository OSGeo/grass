# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test the RPC server base class"""

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
