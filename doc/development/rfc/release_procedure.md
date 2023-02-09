# RFC 4: Release Procedure

Author: GRASS GIS PSC

Contact: [grass-psc AT lists.osgeo.org](http://lists.osgeo.org/mailman/listinfo/grass-psc)

Status: Late draft (7 Jan 2015)

## Summary

In order to render the release process more fluid, to avoid very long release candidate (RC) periods and to lessen the potential for conflict between developers during release preparations, this RFC defines a procedure that should be followed for each release.

## General philosophy

* The release process should be _short_. I.e. the time between the first RC and the final release should be a matter of weeks not many months.
* A release period should be a time of concerted action during which all developers give priority to the release instead of other developments.
* All developers respect a call for commit freeze during a release process.
* It is sometimes better to ship a release with a known bug than with unknown consequences of an untested bug fix.

## General Procedure

Step 1 - Proposal of release:
   When a developer feels that it is time for a new release, she or he should propose the launch of a new release process on the developers mailing list ([grass-dev AT lists.osgeo.org](http://lists.osgeo.org/mailman/listinfo/grass-dev)). The Project manager (or the Release manager, if exists) then collects reactions and decides whether there is sufficient support for this proposal.

Step 2 (day X) - Soft freeze of release branch:

* If support is lacking, a list of outstanding issues (managed via <https://github.com/OSGeo/grass/issues>) that need to be solved before a soft freeze should be sent to the developers mailing list.
* If sufficient support is present, the first announcement is sent by the Release manager to the developers mailing list about the upcoming release along with a trac planning page (section).
The immediate effect of this announcement is a soft freeze, meaning that commits should be limited to non-invasive backports from the development branch/trunk.
The announcement should also include an approximate time table for the release, including the start of hard freeze, RC1, RC2, final release and the link to the trac page. Sufficient time should be left between the soft freeze and the hard freeze. Any backports during the soft freeze should be announced on the developers mailing list with 24 hours advance to allow possible discussion.

Step 3 (X+30 days) - Hard freeze & RC1:
   Once all necessary backports are done, a hard freeze is announced by the Release manager and RC1 is released based on the frozen code (release branch).

Step 4 - Bug squashing:
   All developers concentrate on fixing the remaining bugs during a defined period of no more than 2 weeks. Any commits from that point on can only be well-tested, non-invasive bug fixes.

Step 5 (X+44 days) - RC2:
   RC2 is released as almost final.

Step 6 - Bug squashing & Release preparation:
   A final, concerted bug squashing effort by all developers of no more than one week. During that same time the release announcement is drafted. If an important bug is discovered for which a fix needs some more testing, an RC3 can exceptionally be published, with another week of testing before final release.

Step 7 (X+50 days) - Final release published.
