# RFC 10: C++17 Standard Support for GRASS GIS 8.5

Author of the first draft: Nicklas Larsson

Status: Draft

## Summary

Set new minimal build requirement for C++ code to support C++17 standard. This
supersedes the [RFC 7: Language Standards Support](language_standards_support.md),
regarding C++ standard support.

## Background

The RFC 7, which set the C++ minimal standard support to C++11, was adopted 3.5
years ago. At that time, although the latest versions of common compilers had
full support for C++17, GRASS GIS code itself nor its dependencies were not in
need or required it. Now, compilers with full C++17 support are available also
in what may be considered stable or long-term-support systems.
Moreover, important dependencies such as PDAL 2.4 (released in Mars 2022) and
GDAL 3.9 (May 2024) require C++17 support, and so will the future release of
GEOS 3.14 do.

## C++17 standard for GRASS GIS

The time has come to increase the minimal C++ standard support for GRASS GIS
code to the C++17 standard. This enables use of new C++17 features, if and when
so is needed or recommended. There is, however, no need to make any immediate
changes to current code. Continuous integration runners already compiles in
C++17 mode, making sure nothing will be broken.

## References on C++17 features and support

- [Wikipedia](https://en.wikipedia.org/wiki/C%2B%2B17)
- [cppreference.com](https://en.cppreference.com/w/cpp/17)
