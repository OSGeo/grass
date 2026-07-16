<!-- markdownlint-disable-file line-length -->
# RFC 9: Version Numbering

Author: Vaclav Petras

Status: Adopted (5 June 2023)

## Summary

Use version number in a format _major.minor.micro_ where

- _major_ is for large additions or backwards incompatible changes,
- _minor_ is for additions, and
- _micro_ is for fixes.

These numbers are assigned to releases. Development versions, pre-release,
and builds are marked by additional labels as suffixes.

Once a version number is assigned to a release, the associated source code
cannot change, or in other words, version number is assigned to a particular
state of the source code. This is sometimes called a hard freeze (or code
freeze). Unlike soft freeze (or feature freeze) and branching, code freeze
does not allow any changes of the code.

All major and minor version numbers are used for releases,
i.e., there is no distinction between even and odd numbers.

## Background

The increasing versions were always following the same ideas as in
the [Semantic Versioning](https://semver.org/),
i.e., _major_, _minor_, and _patch_,
with major for breaking changes, minor for backwards compatible
features, and patch for fixes.

For the versions 7.0 till 8.2, there was a distinction between even
and odd minor versions.
Even minor versions were released, while odd minor versions marked development versions.
These odd versions never had a patch number assigned and were never released.

The practice of odd minor versions denoting development versions and even minor versions
denoting releases was introduced with 5.0.0 and was followed in various ways in the
version 5 and 6 series.
This odd-even practice followed the numbering scheme of the Linux kernel
which abandoned the practice since then.

At the time of version 5, the odd-even practice replaced a system where multiple
numbered beta versions were released. Version 5.0beta10 was the last beta
release before the first pre-release of 5.0.0.

The beta version practice was shortly picked up again for 7.0.0 which had four
beta releases before the first release candidate of 7.0.0.

## Motivation

Explaining the version numbering should be as easy as possible.
Ideally, it would not need any explanation at all.
When all version numbers refer to releases, previews (alphas, betas, daily builds)
always need additional indication of the version being a preview.
However, using odd versions to mark the development versions still requires
additional explanation, for example, download for 8.1 on the website
said _preview_ anyway because the odd number does not indicate a
development version by itself, i.e., it's not self-explanatory.
Hence, odd numbers for development versions do not bring any advantage.

The branching and release procedure with even minor numbers for releases
and odd numbers for development versions require that when a new branch for
a minor version is created, the branch needs to change all the mentions
of the minor version number to the next even number and, at the same time,
the _main_ branch needs to change to the next odd number.
Without the even and odd distinction, the branch keeps the version from
the main branch while the main branch advances to the next upcoming version
resulting only in one operation.

The Semantic Versioning uses labels after the version number to indicate development
versions, so using that system or a similar one should be sufficient to mark the
development versions.

## Version Numbering Specification

### Format

Major, minor, and micro versions are separated by periods (dots),
i.e., _major.minor.micro_. The format for development versions and
build information is described in their respective sections.

### Major

In accordance with Semantic Versioning, a major release must happen with
any backwards incompatible change in the API which includes
both interface and behavior changes (API also defines what happens,
not just names and signatures).

Additionally, major feature additions which would require only a minor release
in terms of API stability, are strongly suggested to trigger a major release
as well. This in turn helps to address the issue of outdated tutorials and
small, but breaking, changes.

Some major features, such as changes in the GUI, are fully backwards compatible
(GUI API is not a public API in versions 7 and 8), but major features may
heavily influence tutorials and other teaching materials. In that case,
increasing a major version should be considered given that,
in a sense, behavior linked to a particular interface is changed.

There is always a list of many small changes which are not backwards compatible,
for example, a cleanup of deprecated functions. None of these changes alone
seems worth a major release, but because it would require one, it is never done.
With more common major releases, small changes can happen more often.

### Minor

Minor version must be incremented if new functionality is added
or if existing functionality is marked as deprecated.
Minor version increment is strongly recommended for all new functionality or improvements
which are not bug fixes.

### Micro

If only backwards compatible bug fixes, i.e., fixes of incorrect behavior, are applied,
a micro version can be released.

The micro version is also known as _patch_ (which is what Semantic Versioning
is using) and _point_ (which is what was used in some GRASS GIS documents).
The word _micro_, rather than patch, is used
to avoid collision with patch referring to an individual changeset or fix
(a release contains one or more of these changes).
A point release (or a _dot_ release) can generally apply to anything after
the first dot, i.e., minor or micro release, while
using minor and micro does not have that ambiguity.
Although minor and micro have potential for confusion due to the
similarity of their names, minor is an established term in this context
and micro is sometimes used in this context and in other contexts
(e.g., micro donations), so it has the right connotations.

### Development Versions

Development versions have the version number of the next release which will be released
from a given code base which is defined by a branch.

Development versions of source code on each branch have a dev suffix,
e.g., 3.5.1-dev.
Transition to the Semantic Versioning style is strongly recommended
which means including dash (hyphen) before dev, e.g., 3.5.1-dev.
Notably, these dev-suffixed version numbers are not unique, i.e.,
multiple source code versions are marked the same.

Release candidates (RCs) are pre-releases marked by appending RC and
a release candidate number to the version, e.g., 3.5.1RC2.

Transition to the Semantic Versioning style for pre-releases is strongly recommended.
Version number is followed by a dash (hyphen) followed by a dot-separated identifier
which consists of identification of pre-release type, i.e., rc or RC, optional dot,
and a release candidate number, e.g., 3.5.1-rc.2, 3.5.1-RC.2, 3.5.1-rc2, or 3.5.1-RC2.

Daily builds and other builds of development versions other than release candidates
may use additional dashes to specify the actual source version, e.g.,
3.5.1-dev-05e5df2e7, or day, e.g., 3.5.1-dev-2023-05-29.

Any version parsing or build system must support both systems, the one without
a dash and the one with a dash.

### Build Information

When build information is captured in the version number, Semantic Versioning
prescribes a plus sign as a separator, e.g., 3.5.1+1 or 3.5.1-RC1+1.
However, filenames with a plus sign may not work well, so a dash as a separator
is allowed too while keeping in mind that automated semantic version tools
won't parse the version correctly. A suggested workaround is to use the dash
only in a file name and use plus sign everywhere else.

Any version parsing or build system must support the form with a dash
and the form with a plus sign.

While the assignment of a version is typically done by tagging in a version
control system, so the assumption is that there is always a related tag,
the builds are managed separately, so the assumption is there is no tag in
the version control system.

## Usage

Version numbers should be presented in their specified format.
When appropriate, a shorter version can be used, for example 4
to refer to the whole series or 4.3 to refer to the latest releases
(regardless of the current micro version).

Leaving out the periods (dots) from the version numbers and combining
major, minor, and micro into a single number is discouraged because of
the lack of clarity for humans and because of the
ambiguity for parsing (35 can be version 3.5 or 35).

Version should be considered a separate item from the name.
The name of the project and software is GRASS GIS, not GRASS GIS 8.
So, don't use "GRASS GIS 8 includes foo and bar" when you simply mean
"the current version includes foo and bar" or "GRASS GIS includes foo and bar".

In documentation, the version is often really needed, but don't say
"the default database driver in GRASS GIS 7 is SQLite", instead say
"the default database driver in GRASS GIS is SQLite (since version 7)."
This way the version number in the sentence will always be valid because
the version when the change was introduced stays the same and does not change.

When a version is part of an output or displayed to the user, the version
number should be determined dynamically, not hardcoded, even if it is just
the major version number.

Don't include the version number where it is not needed, for example, text for links
in a release announcement doesn't need a version because version is already given
by the context.

In short comments, references using GN where N is major version number are not
common in general. When the version is important to mention, use vN which is
a common practice.

## Relation to Other Documents

- [RFC 4: Release Procedure](https://trac.osgeo.org/grass/wiki/RFC/4_ReleaseProcedure):
  This RFC describes changes to the numbering. RFC 4 describes the release procedure.
- [Release Schedule](https://trac.osgeo.org/grass/wiki/Release/Schedule)
  (at Trac wiki under Release): The Release Schedule document describes
  schedule, branching, release maintenance, and numbering. The numbering is
  changed, specifically the use of odd version numbers for development.
- [Semantic Versioning](https://semver.org/) (version 2.0.0 at the time of
  writing): Semantic Versioning treatment of _major_, _minor_, and _micro_
  numbers should be respected. The labeling of other versions does not comply
  with Semantic Versioning, but it is a desired state for the future.

## Other Projects

GDAL and PROJ follow the Semantic versioning. QGIS does as well, but in
combination with odd numbers marking the development versions.

Ubuntu and Black lock their version numbering with the release schedule.
Black, after transitioning from beta, releases a major release yearly in January
using the last two digits of year as major version and month as minor release.

## Historical Documents

- Glynn Clements (2007). GRASS-dev GRASS 6.3.0 release preparation.
  Aug. 12 18:12:32 EDT 2007. <https://lists.osgeo.org/pipermail/grass-dev/2007-August/032705.html>
- Neteler, Markus (2001). Towards a stable open source GIS: Status and future
  directions in GRASS development. Second Italian GRASS Users Meeting,
  University of Trento, Feb. 1-2 2001. <https://www.academia.edu/download/5140572/10.1.1.16.8991.pdf>
