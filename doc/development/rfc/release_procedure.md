# RFC 4: Release Procedure

Author: Vaclav Petras

Status: Draft

## Summary

In order to render the release process more fluid, to avoid very long release
candidate (RC) periods and to lessen the potential for conflict between
developers during release preparations, this RFC defines a procedure that
should be followed for each release.

## General Philosophy

* The release process should be _short_, i.e. the time between the first RC
  and the final release should be a matter of weeks not many months.
* A release period should be a time of concerted action during which all
  developers give priority to the release instead of other developments.
* All developers respect a call for commit freeze during a release process
  or any similar policy.
* It is sometimes better to ship a release with a known bug than with unknown
  consequences of an untested bug fix.

## General Procedure

### Step 1 - Proposal of a New Release

The release manager announces upcomming release on the developer mailing list
([grass-dev](http://lists.osgeo.org/mailman/listinfo/grass-dev)) according
to the release schedule or when the release manager deems necessary to create
a new release.

Alternatively, when any contributor feels a need for a new release, the contributor
should propose a creation of a new release on the developer mailing list.
The release manager then collects reactions and decides whether
there is sufficient support for this proposal consiudering also any applicable
release policies and schedules. In rare cases, when there is a lack of consensus,
the release manager or the proposing contributor may refer to the PSC
for a final decision.

### Step 2 (day X + 0) - Soft Freeze of Release Branch

* If support is lacking, a list of outstanding issues (managed via <https://github.com/OSGeo/grass/issues>)
  that need to be solved before a soft freeze should be sent to the developer
  mailing list.
* If sufficient support is present, the first announcement is sent by the
  Release manager to the developers mailing list about the upcoming release
  along with a trac planning page (section).
  The immediate effect of this announcement is a soft freeze, meaning that
  commits should be limited to non-invasive backports from the development
  branch/trunk. The announcement should also include an approximate time table
  for the release, including the start of hard freeze, RC1, RC2, final release
  and the link to the trac page. Sufficient time should be left between the
  soft freeze and the hard freeze. Any backports during the soft freeze should
  be announced on the developers mailing list with 24 hours advance to allow
  possible discussion.

### Step 3 (X + 30 days) - Hard Freeze & RC1

Once all necessary backports are done, a hard freeze is announced by the
Release manager and RC1 is released based on the frozen code (release branch).

### Step 4 - Bug Squashing

All developers concentrate on fixing the remaining bugs during a defined
period of no more than 2 weeks. Any commits from that point on can only be
well-tested, non-invasive bug fixes.

### Step 5 (X + 44 days) - RC2

RC2 is released as almost final.

### Step 6 - Bug Squashing & Release preparation

A final, concerted bug squashing effort by all developers of no more than
one week. During that same time the release announcement is drafted. If an
important bug is discovered for which a fix needs some more testing, an RC3
can exceptionally be published, with another week of testing before final
release.

### Step 7 (X + 50 days) - Final Release Published

## Relation to Other Documents

* [RFC: Version Numbering](https://github.com/wenzeslaus/grass/blob/main/doc/development/rfc/version_numbering.md):
  The Version Numbering RFC describes how these version are numbered.
  This RFC describes how to release a new version of the software.
* RFC: Release Policy: The Release Policy RFC describes what releases should be prepared and when.
  This RFC describes how to do a particular release.
