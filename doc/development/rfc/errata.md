# RFC 5: Errata

Status: Draft

## Summary

Some bugs make GRASS produce results which are wrong but look plausible. Users
who already ran an affected analysis, and published or acted on its results,
need a way to find out. This RFC describes how such fixes are marked and how
the resulting errata reach users.

## What qualifies as an erratum

An erratum is issued for a bug which

* is present in an official GRASS release, and
* causes incorrect analysis results which are not easy to notice.

Crashes, error messages and obviously wrong output are ordinary bugs, not
errata, because users can see that something went wrong. Typical errata are a
single cell shift of a raster result, loss of precision due to incorrect
floating point handling, or a wrong conversion factor. A bug introduced and
fixed between two releases does not need an erratum, since no release is
affected.

## Process

1. The pull request fixing the bug gets the `errata` label. Anyone can add it:
   the author, a reviewer, or whoever merges the fix. There is no separate
   nomination or approval step. When it is not clear whether a fix qualifies,
   it is discussed in the pull request like any other review question.

2. The pull request description explains the problem for users, not only for
   developers: which tools are affected, which released versions are affected,
   what was computed incorrectly, and how users can tell whether their own
   results are affected. The person who fixed the bug writes this while the
   details are fresh.

3. The fix is backported to the supported release branches so that users
   receive it. When it cannot be backported, the description says which
   versions stay affected.

4. The release notes of the release containing the fix get an `Errata` section
   listing the labeled fixes, using the user-facing wording from step 2 and
   linking to the pull request. The
   [8.4.2 release notes](https://github.com/OSGeo/grass/releases/tag/8.4.2)
   are an example.

The list of errata across releases is the
[merged pull requests with the errata label](https://github.com/OSGeo/grass/pulls?q=is%3Apr+is%3Amerged+label%3Aerrata).
