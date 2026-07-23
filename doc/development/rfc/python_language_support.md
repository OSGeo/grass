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

Given that addons need to work at least with the latest release and the
development version, the minimal Python version in grass-addons is not raised
together with the development version. It is raised only after the next
release.

For example, suppose the latest release is GRASS 8.3 with a minimal Python
version of 3.8, 8.4 is the development version, and Python 3.8 reaches end of
life before 8.4.0 is released. We raise the minimal Python version for 8.4.0 to
3.9 in the requirements file right away. The grass8 branch in grass-addons repo
has to keep working with the 8.3 release and with the development version, so
its minimal Python version stays at 3.8 until 8.4.0 is released and is raised to
3.9 only then.

### References

* Sunsetting Python 2: <https://www.python.org/doc/sunset-python-2/>
* Active Python Releases: <https://www.python.org/downloads/>
* Version list: <https://devguide.python.org/versions/>
