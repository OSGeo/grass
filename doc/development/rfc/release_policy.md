# RFC: Release Policy

Author: Vaclav Petras

Status: Draft

## Goal

The goal of this document is to define when releases should be created
and what types of releases should be created.

## Summary

One major or minor release should be released per year.
This becomes the only actively maintained release series
which is subsequently updated by micro releases as needed.
Individual contributors can step up to maintain an older release series.

## Types of Releases

We recognize major, minor, micro releases and additional helper versions
each with its unique purpose.

The releases follow rules and numbering specified in the Version Numbering RFC.

### Major and Minor

Both major and minor releases bring new features to the users
and in that regard should receive similar attention by the contributors.
However, they differ in how backwards compatibility is handled.

While major release brings features just like a minor release,
it may also bring changes breaking backwards compatibility
such as breaking API changes. Additionally, contributors may decide
to include certain feature into a major release rather than a minor release
if the feature has a potential to negatively impact some users or use cases.
A major release may include a particularly important feature or a set of
features, but that's not required. The purpose of a major release is to bring
changes which cannot be included into a minor release such as
backwards-incompatible changes in API or unusually large code rewrites.
It may or may not be used to bring revolutionary changes or highlight specific
features.

Minor release may bring any new features, small or big, as long as backwards
compatibility is kept, e.g., an automated procedure relying on public API
still works. The purpose of a minor release is to bring features to the users
as soon as possible with software stability and compatibility, but without
a need to wait for a major release.

Both major and minor releases may include bugfixes which may or may not be
already included in a micro release of the previous major or minor release.

A special effort is put into keeping backwards compatibility.
While this is a requirement for minor versions, the community makes extra effort
even for major releases to maximize backwards compatibility so that users can
conveniently transition to new versions. This is, however, not guaranteed and
it depends on many factors.

### Micro

Micro releases should contain only bugfixes.
Their purpose is to fix issues with a particular major or minor release.
They provide software stability and fixes to the users.

A minor release should be considered instead of a micro release when
a bugfix is large, a bugfix has unclear side-effects, or it is not clear
whether a particular change is bugfix or a feature given that, in practice,
the line between a bugfix and a feature is not completely clear.

### Pre-releases and Daily Builds

There can be zero or more release candidates, i.e., pre-releases,
for each major, minor, or micro release. The specifics are determined
by the Release Procedure RFC.

There can be daily or per-commit frequent builds 
which can serve as preview versions or versions for testing.
Other frequencies are permitted but not recommended.

### Post-releases and Build Versions

There can be subsequent builds of a specific release
if build or packaging process failed and needs fixing.
However, there shouldn't be any post-releases, i.e.,
if there is a change in the code base to fix a flaw in a release,
a new micro release should be created.

## Frequency

One major or minor release should be released at least once a year.
More than one major or minor release may be released
if the contributors consider that necessary or advantageous.
There is no frequency requirement for major releases, but
the practice and rhythm so far was to release a new major version
before the minor version number reaches the value ten which translates
to a major release every five to ten years.

There can be zero or more micro releases per year for different major or minor
releases based on the needs of the community and practically driven by the number
of fixes accumulated.

## Maintenance

At any time there is one actively updated, maintained, and supported release series
which is started by a release of a new major or minor version.
There are micro versions releases in this series throughout the year as needed.

Once a new major or minor version is released, the old release series goes to
a low maintenance mode and no further releases are planned.
However, any low maintenance series can be updated on demand.
If a contributor submits a patch to fix a bug, it will be likely accepted and
a new release will be created when there are enough changes accumulated.
Alternatively the contributor might receive support in creating the release as
a temporary release manager.
Security fixes will be given special consideration in light of how wide
the usage of a particular release series is.

## Relation to Other Documents

* Release procedure covers branching and creation of pre-releases.
* Version number covers how major, minor, micro releases are numbered
  and what types of changes can be included in a release.
