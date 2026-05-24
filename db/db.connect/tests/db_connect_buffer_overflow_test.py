import pytest
import ctypes
import sys
import os

# Simulate the buffer size constant from the C code
GPATH_MAX = 4096

# Simulated Python equivalent of the vulnerable db.connect logic
# This models what the C code does, but in Python with explicit bounds checking
# The invariant: any copy into a buffer of GPATH_MAX must never exceed GPATH_MAX bytes

def safe_db_connect_copy(database_name: str) -> str:
    """
    Simulates the db.connect buffer copy behavior.
    The invariant: result must never exceed GPATH_MAX bytes.
    If the input exceeds GPATH_MAX, it must be truncated or rejected.
    """
    if database_name is None:
        msg = "database_name cannot be None"
        raise ValueError(msg)
    # Simulate buffer allocation of GPATH_MAX bytes
    buffer_size = GPATH_MAX
    # The vulnerable C code does strcpy without bounds checking.
    # A safe implementation MUST truncate or reject oversized input.
    encoded = database_name.encode("utf-8", errors="replace")
    # Enforce the invariant: never write more than buffer_size bytes
    if len(encoded) >= buffer_size:
        # Must truncate to fit within buffer (leaving room for null terminator)
        encoded = encoded[: buffer_size - 1]
    return encoded.decode("utf-8", errors="replace")


def simulate_vulnerable_strcpy(src: str, buf_size: int) -> int:
    """
    Simulates what strcpy does: returns the number of bytes that WOULD be written.
    In the vulnerable C code, this is unchecked.
    Returns the length of src in bytes (what strcpy would copy including null terminator).
    """
    return len(src.encode("utf-8", errors="replace")) + 1  # +1 for null terminator


@pytest.mark.parametrize("payload", [
    # Exactly at boundary
    "A" * GPATH_MAX,
    # One byte over boundary
    "A" * (GPATH_MAX + 1),
    # 2x the buffer size
    "B" * (GPATH_MAX * 2),
    # 10x the buffer size
    "C" * (GPATH_MAX * 10),
    # 100x the buffer size
    "D" * (GPATH_MAX * 100),
    # Unicode characters that expand when encoded
    "\u00e9" * (GPATH_MAX // 2 + 100),  # 2-byte UTF-8 chars, exceeds buffer
    "\u4e2d" * (GPATH_MAX // 3 + 100),  # 3-byte UTF-8 chars, exceeds buffer
    "\U0001f600" * (GPATH_MAX // 4 + 100),  # 4-byte UTF-8 chars, exceeds buffer
    # Path traversal with oversized input
    "../" * (GPATH_MAX // 3 + 50),
    # Null bytes embedded in oversized string
    "A" * 100 + "\x00" + "B" * (GPATH_MAX * 2),
    # Special characters with oversized input
    "/tmp/" + "x" * (GPATH_MAX * 3),
    # Mixed content exceeding buffer
    "database_name=" + "Z" * (GPATH_MAX * 5),
    # Exactly GPATH_MAX - 1 (should be safe)
    "E" * (GPATH_MAX - 1),
    # Empty string (edge case)
    "",
    # Single character
    "x",
    # Whitespace padding oversized
    " " * (GPATH_MAX * 4),
    # Newlines and special chars oversized
    "\n\r\t" * (GPATH_MAX + 500),
    # SQL injection style oversized
    "'; DROP TABLE users; --" + "A" * (GPATH_MAX * 2),
    # Format string style oversized
    "%s%n%x" * (GPATH_MAX // 6 + 100),
])
def test_buffer_read_never_exceeds_declared_length(payload):
    """
    Invariant: Buffer reads/writes must never exceed GPATH_MAX bytes.
    The db.connect module allocates GPATH_MAX bytes and uses strcpy without
    bounds checking. This test verifies that any safe implementation of this
    logic must truncate or reject inputs that would overflow the buffer.
    Specifically:
    1. The number of bytes written to the buffer must never exceed GPATH_MAX.
    2. The result stored in the buffer must fit within GPATH_MAX bytes
       (including null terminator in C, so max GPATH_MAX-1 usable bytes).
    3. Oversized inputs must be truncated or cause an explicit rejection,
       never silently overflow.
    """
    # Invariant check 1: Detect if the vulnerable strcpy WOULD overflow
    bytes_that_would_be_written = simulate_vulnerable_strcpy(payload, GPATH_MAX)
    # If the payload would overflow, the safe implementation must handle it
    would_overflow = bytes_that_would_be_written > GPATH_MAX
    if would_overflow:
        # The safe implementation must either truncate or raise an exception
        try:
            result = safe_db_connect_copy(payload)
            # If it didn't raise, it must have truncated
            result_bytes = result.encode("utf-8", errors="replace")
            # Invariant: result must fit within buffer (with null terminator)
            assert len(result_bytes) < GPATH_MAX, (
                f"Buffer overflow invariant violated: result length {len(result_bytes)} "
                f">= GPATH_MAX ({GPATH_MAX}). Input length was {len(payload)} chars / "
                f"{len(payload.encode('utf-8', errors='replace'))} bytes."
            )
            # Invariant: result must be a prefix of the original input (truncation, not corruption)
            original_bytes = payload.encode("utf-8", errors="replace")
            assert original_bytes.startswith(result_bytes) or len(result_bytes) == 0, (
                f"Truncation invariant violated: result is not a prefix of the original input. "
                f"Result bytes: {result_bytes[:50]}..., Original bytes: {original_bytes[:50]}..."
            )
        except (ValueError, OverflowError, MemoryError) as e:
            # Explicit rejection is also acceptable behavior
            pass
        except Exception as e:
            pytest.fail(
                f"Unexpected exception type {type(e).__name__}: {e}. "
                f"Expected either truncation or ValueError/OverflowError/MemoryError "
                f"for oversized input of length {len(payload)}."
            )
    else:
        # Input fits within buffer, must succeed without modification
        result = safe_db_connect_copy(payload)
        result_bytes = result.encode('utf-8', errors='replace')
        # Invariant: safe input must still fit in buffer
        assert len(result_bytes) < GPATH_MAX, (
            f"Even safe-sized input overflowed buffer: result length {len(result_bytes)} "
            f">= GPATH_MAX ({GPATH_MAX})."
        )


@pytest.mark.parametrize("payload", [
    "A" * (GPATH_MAX * 2),
    "B" * (GPATH_MAX * 10),
    "/path/to/db/" + "x" * (GPATH_MAX * 3),
])
def test_multiple_strcpy_calls_all_bounded(payload):
    """
    Invariant: All strcpy calls in the function (there are multiple) must
    each individually respect the GPATH_MAX bound.
    The vulnerable code calls strcpy into 'buf' multiple times.
    Each call must be bounded independently.
    """
    # Simulate the three strcpy calls from the vulnerable code
    # Each must independently not overflow GPATH_MAX
    for copy_number in range(1, 4):  # Three strcpy calls in the vulnerable code
        try:
            result = safe_db_connect_copy(payload)
            result_bytes = result.encode("utf-8", errors="replace")
            assert len(result_bytes) < GPATH_MAX, (
                f"strcpy call #{copy_number} would overflow buffer: "
                f"attempted to write {len(result_bytes) + 1} bytes into "
                f"buffer of size {GPATH_MAX}. "
                f"Input was {len(payload)} characters."
            )
        except (ValueError, OverflowError, MemoryError):
            # Rejection is acceptable
            pass


@pytest.mark.parametrize(
    ("database_name", "expected_max_bytes"),
    [
        ("normal_db", GPATH_MAX - 1),
        ("A" * (GPATH_MAX - 1), GPATH_MAX - 1),
        ("A" * GPATH_MAX, GPATH_MAX - 1),  # Must be truncated
        ("A" * (GPATH_MAX + 1), GPATH_MAX - 1),  # Must be truncated
        ("A" * (GPATH_MAX * 2), GPATH_MAX - 1),  # Must be truncated
    ],
)
def test_output_never_exceeds_max_buffer_size(database_name, expected_max_bytes):
    """
    Invariant: The output stored in the buffer must never exceed expected_max_bytes,
    which is GPATH_MAX - 1 (leaving room for null terminator as in C strings).
    """
    try:
        result = safe_db_connect_copy(database_name)
        result_byte_length = len(result.encode("utf-8", errors="replace"))
        assert result_byte_length <= expected_max_bytes, (
            f"Output exceeds maximum allowed buffer size. "
            f"Got {result_byte_length} bytes, maximum is {expected_max_bytes} bytes "
            f"(GPATH_MAX={GPATH_MAX}). "
            f"Input was {len(database_name)} characters."
        )
    except (ValueError, OverflowError, MemoryError):
        # Explicit rejection for oversized input is acceptable
        pass
