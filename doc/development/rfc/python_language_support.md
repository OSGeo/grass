# RFC 8: Python Language Support

Authors: Nicklas Larsson

Status: Adopted (5 June 2023)

## Introduction

The code base of GRASS GIS consists as of Feb. 2021 of predominantly C code
(ca 50 %), Python (ca 30 %) and a smaller amount of C++ (ca 5 %). Each of these
languages have evolved significantly in the last 10–20 years. There is, however,
no clearly stated policy of supported language standard(s), nor mechanism to
update this policy when needed or wanted. This result in uncertainty for
contributors for what may be allowed and solutions that may not be optimal.

This RFC aims at setting a policy for GRASS GIS project regarding the minimum
version support for Python.

## Background

Similarly, the integration of Python within GRASS GIS – driving the GUI and
to a large extent replacing shell scripts (bash/sh) for scripting – has gone
through a challenging period with the transition from Python 2 to 3. This
transition can now be considered completed, but this fact need to be formalised
and the minimum version of Python 3 support must be stated.

The minimum support for Python will, in contrast, likely change more often,
due to each (minor) version’s 5-years life-time. At this date, the oldest
still-alive Python version is 3.7, to be retired in June 2023.

## Version Updates

For a new release of a minor GRASS version, the Python minimum version should
be raised in the [requirements](https://github.com/OSGeo/grass/blob/main/REQUIREMENTS.md#general-requirements)
file if the current minimum Python version reaches end of life or there are any
important technical reasons.

At the same time, we raise the minimal Python version for a matching branch
in grass-addons.

For example, when Python 3.8 goes end-of-life before the release of 8.4.0, we
raise the minimal Python version for 8.4.0 to 3.9. We also raise the minimal
Python version for grass8 branch in grass-addons repo to Python 3.9.

### References

* Sunsetting Python 2: <https://www.python.org/doc/sunset-python-2/>
* Active Python Releases: <https://www.python.org/downloads/>
* Version list: <https://devguide.python.org/versions/>
