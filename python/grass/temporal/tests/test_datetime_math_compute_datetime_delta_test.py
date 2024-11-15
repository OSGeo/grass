from datetime import datetime
from grass.temporal.datetime_math import compute_datetime_delta, datetime_delta


def test_datetime_delta_typeddict_dict() -> None:
    assert datetime_delta(
        month=3, day=2, minute=5, year=1, hour=8, max_days=455, second=30
    ) == dict(  # noqa: C408
        month=3, day=2, minute=5, year=1, hour=8, max_days=455, second=30
    )


def test_datetime_delta_typeddict_dict_literal() -> None:
    assert datetime_delta(
        month=3, day=2, minute=5, year=1, hour=8, max_days=455, second=30
    ) == {
        "month": 3,
        "day": 2,
        "minute": 5,
        "year": 1,
        "hour": 8,
        "max_days": 455,
        "second": 30,
    }


def test_datetime_delta_typeddict_dict_different_order() -> None:
    assert datetime_delta(
        month=3, day=2, minute=5, year=1, hour=8, max_days=455, second=30
    ) == {
        "max_days": 455,
        "month": 3,
        "minute": 5,
        "year": 1,
        "day": 2,
        "hour": 8,
        "second": 30,
    }


def test_datetime_delta_typeddict_typeddict_different_order() -> None:
    assert datetime_delta(
        year=1,
        month=3,
        day=2,
        hour=8,
        minute=5,
        second=30,
        max_days=455,
    ) == datetime_delta(
        month=3,
        day=2,
        minute=5,
        year=1,
        hour=8,
        max_days=455,
        second=30,
    )


def test_datetime_delta_typeddict_dict_missing() -> None:
    assert datetime_delta(
        month=3, day=2, minute=5, year=1, hour=8, max_days=455, second=30
    ) != {"month": 3, "day": 2, "minute": 5, "hour": 8, "max_days": 455}


def test_compute_datetime_delta_same_date() -> None:
    start = datetime(2001, 1, 1, 0, 0, 0)
    end = datetime(2001, 1, 1, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=0, minute=0, second=0, max_days=0
    )


def test_compute_datetime_delta_seconds_only() -> None:
    start = datetime(2001, 1, 1, 0, 0, 14)
    end = datetime(2001, 1, 1, 0, 0, 44)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=0, minute=0, second=30, max_days=0
    )


def test_compute_datetime_delta_minute_seconds() -> None:
    start = datetime(2001, 1, 1, 0, 0, 44)
    end = datetime(2001, 1, 1, 0, 1, 14)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=0, minute=1, second=30, max_days=0
    )


def test_compute_datetime_delta_minutes_seconds_same() -> None:
    start = datetime(2001, 1, 1, 0, 0, 30)
    end = datetime(2001, 1, 1, 0, 5, 30)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=0, minute=5, second=300, max_days=0
    )


def test_compute_datetime_delta_minute_only() -> None:
    start = datetime(2001, 1, 1, 0, 0, 0)
    end = datetime(2001, 1, 1, 0, 1, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=0, minute=1, second=0, max_days=0
    )


def test_compute_datetime_delta_hour_minutes_same() -> None:
    start = datetime(2011, 10, 31, 0, 45, 0)
    end = datetime(2011, 10, 31, 1, 45, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=1, minute=60, second=0, max_days=0
    )


def test_compute_datetime_delta_hour_minutes() -> None:
    start = datetime(2011, 10, 31, 0, 45, 0)
    end = datetime(2011, 10, 31, 1, 15, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=1, minute=30, second=0, max_days=0
    )


def test_compute_datetime_delta_hours_minutes() -> None:
    start = datetime(2011, 10, 31, 0, 45, 0)
    end = datetime(2011, 10, 31, 12, 15, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=12, minute=690, second=0, max_days=0
    )


def test_compute_datetime_delta_hour_only() -> None:
    start = datetime(2011, 10, 31, 0, 0, 0)
    end = datetime(2011, 10, 31, 1, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=1, minute=0, second=0, max_days=0
    )


def test_compute_datetime_delta_month_day_hour() -> None:
    start = datetime(2011, 10, 31, 0, 0, 0)
    end = datetime(2011, 11, 1, 1, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=1, hour=25, minute=0, second=0, max_days=1
    )


def test_compute_datetime_delta_month_day_hours() -> None:
    start = datetime(2011, 10, 31, 12, 0, 0)
    end = datetime(2011, 11, 1, 6, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=0, hour=18, minute=0, second=0, max_days=0
    )


def test_compute_datetime_delta_month_hour() -> None:
    start = datetime(2011, 11, 1, 0, 0, 0)
    end = datetime(2011, 12, 1, 1, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=1, day=0, hour=721, minute=0, second=0, max_days=30
    )


def test_compute_datetime_delta_days_only() -> None:
    start = datetime(2011, 11, 1, 0, 0, 0)
    end = datetime(2011, 11, 5, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=4, hour=0, minute=0, second=0, max_days=4
    )


def test_compute_datetime_delta_month_days_less_than_month() -> None:
    start = datetime(2011, 10, 6, 0, 0, 0)
    end = datetime(2011, 11, 5, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=30, hour=0, minute=0, second=0, max_days=30
    )


def test_compute_datetime_delta_year_month_days_less_than_month() -> None:
    start = datetime(2011, 12, 2, 0, 0, 0)
    end = datetime(2012, 1, 1, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=0, day=30, hour=0, minute=0, second=0, max_days=30
    )


def test_compute_datetime_delta_month_only() -> None:
    start = datetime(2011, 1, 1, 0, 0, 0)
    end = datetime(2011, 2, 1, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=1, day=0, hour=0, minute=0, second=0, max_days=31
    )


def test_compute_datetime_delta_year_month() -> None:
    start = datetime(2011, 12, 1, 0, 0, 0)
    end = datetime(2012, 1, 1, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=1, day=0, hour=0, minute=0, second=0, max_days=31
    )


def test_compute_datetime_delta_year_months() -> None:
    start = datetime(2011, 12, 1, 0, 0, 0)
    end = datetime(2012, 6, 1, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=6, day=0, hour=0, minute=0, second=0, max_days=183
    )


def test_compute_datetime_delta_years_only() -> None:
    start = datetime(2011, 6, 1, 0, 0, 0)
    end = datetime(2021, 6, 1, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=10, month=120, day=0, hour=0, minute=0, second=0, max_days=3653
    )


def test_compute_datetime_delta_year_hours() -> None:
    start = datetime(2011, 6, 1, 0, 0, 0)
    end = datetime(2012, 6, 1, 12, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=12, day=0, hour=8796, minute=0, second=0, max_days=366
    )


def test_compute_datetime_delta_year_hours_minutes() -> None:
    start = datetime(2011, 6, 1, 0, 0, 0)
    end = datetime(2012, 6, 1, 12, 30, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=12, day=0, hour=8796, minute=527790, second=0, max_days=366
    )


def test_compute_datetime_delta_year_hours_seconds() -> None:
    start = datetime(2011, 6, 1, 0, 0, 0)
    end = datetime(2012, 6, 1, 12, 0, 5)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=12, day=0, hour=8796, minute=0, second=31665605, max_days=366
    )


def test_compute_datetime_delta_year_minutes() -> None:
    start = datetime(2011, 6, 1, 0, 0, 0)
    end = datetime(2012, 6, 1, 0, 30, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=12, day=0, hour=0, minute=527070, second=0, max_days=366
    )


def test_compute_datetime_delta_year_seconds() -> None:
    start = datetime(2011, 6, 1, 0, 0, 0)
    end = datetime(2012, 6, 1, 0, 0, 5)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=1, month=12, day=0, hour=0, minute=0, second=31622405, max_days=366
    )


def test_compute_datetime_delta_month_full() -> None:
    start = datetime(2011, 7, 3, 0, 0, 0)
    end = datetime(2011, 8, 2, 0, 0, 0)
    comp: datetime_delta = compute_datetime_delta(start, end)
    assert comp == datetime_delta(
        year=0, month=0, day=30, hour=0, minute=0, second=0, max_days=30
    )
