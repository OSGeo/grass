# RFC 1: Project Steering Committee Guidelines

Author: [GRASS PSC](https://trac.osgeo.org/grass/wiki/PSC)

Contact: [grass-psc AT lists.osgeo.org](http://lists.osgeo.org/mailman/listinfo/grass-psc)

Status: Adopted (6 April 2007)

## Summary

A GRASS Project Steering Committee ([PSC](https://trac.osgeo.org/grass/wiki/PSC)) is proposed to formalize control
over the GRASS codebase and to facilitate GRASS project management issues.
It is desired to keep the administrational overhead as low as possible.

This document describes how the GRASS Project Steering Committee
determines membership, and makes decisions on GRASS project issues.

"The GRASS Project" is defined as the GPL-licenced GIS software known as the
Geographic Resources Analysis Support System, together with the surrounding
development, distribution and promotion infrastructure currently headquarted
at OSGeo.

## Terms of Reference

The two primary functions of the PSC are:

 1. To enforce control over the GRASS codebase. This can be summarised as:
     * Enforce mechanisms to ensure quality control.
     * Ensure compliance with all required legal measures.
 2. Project Management and responsibility for the "public face" of GRASS.

The PSC is expected to be able to speak and act on behalf of the GRASS
project.

## Codebase Control

### Quality Control Mechanisms

The quality control mechanisms, which are the responsibility of the PSC,
currently include:

* Maintaining submitter guidelines and making all developers aware of them.
* Granting write access to the source code repository for new developers.
* Enforcing the submitter guidelines, with the ultimate sanction against non-compliance being removal of write access to the source code repository.

In general, once write access has been granted, developers are allowed to
make changes to the codebase as they see fit. For controversial or
complicated changes consensus must be obtained on the developers' mailing
list as far as reasonably practicable. It is recognised that the ultimate
arbitration on technical issues should always lie with consensus on the
developers' mailing list. Specifically, it is not the role of the PSC to
impose technical solutions. Its role is in general limited to enforcing the
quality control mechanisms outlined above.

However, if consensus fails to emerge naturally, an issue can be
referred to the PSC for more structured efforts to build consensus.
As a last resort, if lack of consensus continues, the developer
community can request the PSC to choose options best preserving the
quality of the GRASS project.

Removal of write access to the source code repository is handled as a
proposal to the committee as described below in the [Operation of the PSC](#operation-of-the-psc) section.

### Legal aspects

Control over the codebase also extends to ensuring that it complies with
all relevant legal requirements. This includes copyright and licensing
amongst other issues. The PSC is responsible for developing rules and
procedures to cover this. These are outlined in a separate document:
[RFC 2: Legal aspects of code contributions](legal_aspects_of_code_contributions.md).
This document will be updated and revised by the PSC as required.

## Project Management

The PSC will share responsibility and make decisions over issues related
to the management of the overall direction of the GRASS project and
external visibility, etc. These include, but are not limited to:

* Release Cycles
* Project infrastructure
* Website Maintenance
* Promotion and Public Relations
* Other issues as they become relevant

It is the responsibility of the PSC to ensure that issues critical to the
future of the GRASS project are adequately attended to. This may involve
delegation to interested helpers.

## Operation of the PSC

A dedicated [mailing list](https://lists.osgeo.org/mailman/listinfo/grass-psc)
exists for the purpose of PSC discussions. When a
decision is required of the PSC, it will be presented by any member to the
mailing list in the form of a proposal. A decision will then be achieved
by discussion of the proposal on the mailing list until a consensus is
reached. Voting on issues is also permissable and may be used as a means
to reach a consensus or, only in case of extreme cases of disagreement, to
force a decision. Any member may call a vote on any proposal. The voting
procedure is outlined in a separate document:
[RFC 3: PSC Voting Procedures](PSC_voting_procedures.md).

The Chair is the ultimate adjudicator in case of deadlock or irretrievable
break down of decision-making, or in case of disputes over voting.

The following issue(s) *must* have a vote called before a
decision is reached:

* Granting source code repository write access for new developers
* Selection of a committee Chair

## Composition of the Committee

Initial PSC membership was decided based on a nomination and informal voting
period on the community's mailing lists.  Michael Barton, Dylan Beaudette,
Hamish Bowman, Massimiliano Cannata, Brad Douglas, Paul Kelly, Helena Mitasova,
Scott Mitchell, Markus Neteler, and Maciej Sieczka are declared to be the
founding Project Steering Committee.

Addition and removal of members from the committee, as well as selection
of Chair is handled as a proposal to the committee as described above.

The Chair is responsible for keeping track of the membership of the PSC.
