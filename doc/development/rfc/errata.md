# RFC 5: GRASS GIS Errata

Author of the first draft: Māris Nartišs

Status: Early draft (19 Mar 2016)

## Introduction

GRASS GIS is widely used in scientific, private and government sector.
Scientific theories, environmental decisions and actions depend on the outcome
of spatial analysis performed with GRASS GIS. Any errors in analytical modules
might lead to erroneous conclusions and actions based on them. This RFC
provides a framework of documenting and spreading information on analysis
inaccuracies caused by errors in GRASS GIS code.

## Overall process

1. Any bug reporter or developer can nominate a bug for escalating to GRASS GIS
   erratum issue. Nomination is done by adding a notice to bug at bug tracker
   or discussing directly at  developer mailing list. Any nomination is
   discussed in developer mailing list to gather necessary information on it's
   scope, impact, causes and solutions.

2. GRASS PSC evaluates nomination based on information provided by bug report,
   discussion in developer mailing list and/or other sources. PSC makes a final
   decision if nominated bug matches criteria of issuing a GRASS GIS erratum.

3. A draft of erratum text is made in a designated area of developer wiki
   (issue tracker) holding texts of all published GRASS GIS errata.

4. After a review of at least one more person, the erratum is marked as final
   one and spread via communication channels.

### Evaluation criteria

* Bug must be present in an official GRASS GIS release.
* Bug must cause generation of incorrect analysis results that are not so easy
  to notice. Module crashes or bugs causing easy to identify incorrect results
  should not be given an erratum. Examples of possible erratum worth bugs are
  single cell shift of raster result, not enough randomness of expected random
  module output, loss of output precision due to incorrect floating point
  handling etc.

### Content and life cycle of an Erratum

GRASS GIS Erratum message should contain following elements:

* it's number;
* date of issue;
* name(s) of affected module(s);
* information about affected release(s);
* a short description of problem;
* steps resulting in incorrect output (i.e. specific input parameter combination);
* current state of problem (in progress, fixed for release x.y.z);
* references to bug report (Trac bug number), developer mailing list thread;
* any other information relevant to erratum.

GRASS GIS errata might receive updates, if it's found to be necessary
(i.e. notice of fixing issue, issue scope update etc.).

### Spreading the word

All GRASS GIS errata texts should be available at two places - developer wiki
(Trac) and ERRATA file of any upcoming release. Errata should be listed in time
descending order (latest on the top).

If the erratum was based on a bug report in issue tracker, a notice to the
issue report should be added.

Announcement of the erratum should be sent out to XXXX mailing list.
