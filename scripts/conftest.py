"""General pytest test fixtures

Author: Edouard Choinière (2025-03-09)
SPDX-License-Identifier: GPL-2.0-or-later
"""

from collections.abc import Generator

import pytest


@pytest.fixture(scope="module")
def monkeypatch_module() -> Generator[pytest.MonkeyPatch]:
    """Yield a monkeypatch context, through a module-scoped fixture"""
    with pytest.MonkeyPatch.context() as mp:
        yield mp
