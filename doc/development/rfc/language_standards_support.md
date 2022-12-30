# RFC 7: Language Standards Support

Author of the first draft: Nicklas Larsson

Status: Motion passed, 29 March 2021

## Introduction
The code base of GRASS GIS consists today (Feb. 2021) of predominantly C code (ca 50 %), Python (ca 30 %) and a smaller amount of C++ (ca 5 %). Each of these languages have evolved significantly in the last 10–20 years. There is, however, no clearly stated policy of supported language standard(s), nor mechanism to update this policy when needed or wanted. This result in uncertainty for contributors for what may be allowed and solutions that may not be optimal.

This RFC aims at setting a policy for GRASS GIS project regarding language standard support for C and C++.

In addition, this is also intended to set a precedent for future updates on this subject.

## Background
Throughout its long history, soon 40 years, GRASS GIS has evolved and steps have been taken to adapt and modernize. The latest big modernization of the C code was done in 2002–2006 ([summary](https://lists.osgeo.org/pipermail/grass-dev/2021-February/094955.html)), when it was updated to conform to C89 (ANSI C) standard. A major job, which has payed-off well. However, during the years, language features of successive standards have slipped into the code base, which is no longer strictly C89 (nor C90) conformant. There are no compelling reasons to revert the existing code to strict C89, therefore the community has to decide which standard to adhere to.

The small amount of C++ code in GRASS GIS has never been formalised and officially made to conform to any specific standard.

See also the discussion leading to this RFC on the mailing list [thread](https://lists.osgeo.org/pipermail/grass-dev/2021-January/094899.html).

## Discussion
The advantage of having clearly stated policy on language standard requirements/support is important not only for contributors, but also sets the frame for supported platforms. For the latter, also the reverse is true: in deciding supported standard the community needs to consider the degree of support of standards for various platforms.

It should be emphasized that existing GRASS GIS C and C++ code compiles also with C17 and C++17. There is therefore no need to modernize it the way it was done to C in the 2000’s. Nevertheless, conforming to newer standards may provide better cross platform support and possibly safer code.

Regarding C, there are three standards that may be considered: C99, C11 and
C17. C99 never really reached full support on key platforms, this is
particularly the case for Windows
([Visual Studio 2013](https://devblogs.microsoft.com/cppblog/c99-library-support-in-visual-studio-2013/)).
Partly in consequence of this lack of support for some C99 features, the C11
standard was made less strict: making some C99 mandatory features optional.
Thus, from autumn 2020 even
[MSVC complies to C11](https://devblogs.microsoft.com/cppblog/c11-and-c17-standard-support-arriving-in-msvc/)
core feature support. Starting with
[GCC 4.9](https://gcc.gnu.org/wiki/C11Status) it had “substantially
complete” support for C11, Clang from
[version 3.1](https://releases.llvm.org/3.1/docs/ClangReleaseNotes.html).
[C17](https://en.wikipedia.org/wiki/C17_(C_standard_revision)), on the other
hand, doesn’t add new features compared to C11. Its difference is more
interesting from compiler point of view, whereas code “good for C11” is
good for C17.

Regarding C++, there are the C++98, C++03, C++11, C++14 and C++17 standards to consider. The platform and compiler support for all of these are significantly better. However, C++11 is at this date in general considered the standard and until compelling reasons argue otherwise, the C++11 standard should be policy of the GRASS GIS project.

## Proposed Language Standards Support

### C Language
C11 with core (mandatory) features [brief summary](https://en.wikipedia.org/wiki/C11_(C_standard_revision))

Optional features may be used if availability is tested with macro, and if not supported, alternative fallback code must be provided.

### C++ Language
C++11 [summary](https://en.wikipedia.org/wiki/C%2B%2B11)

