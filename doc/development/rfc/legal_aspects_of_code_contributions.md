# RFC 2: Legal aspects of code contributions

Author: Markus Neteler (based on GDAL.org/RFC3)

Contact: [grass-psc AT lists.osgeo.org](http://lists.osgeo.org/mailman/listinfo/grass-psc)

Status: Adopted (8 Dec 2006)

## Legal aspects

GRASS developers have to keep the code base clear of improperly
contributed code. It is important to the GRASS users, developers and
the OSGeo foundation to avoid contributing any code to the project
without it being clearly licensed under the project license or a
compliant license. In this document, a "committer" is understood to be
a developer with write access to the GRASS source code repository.

Generally speaking, the key issues are that those individuals
providing code to be included in the GRASS repository understand that
the code will be released under the GPL >=2 license, and that the
person providing the code has the right to contribute the code.  In
order to verify this, the committer must have a clear understanding of
the license themselves. When committing 3rd party contributions, the
committer should verify the understanding unless the committer is very
comfortable that the contributor understands the license (for instance
frequent contributors).

If the contribution was developed on behalf of an employer (on work
time, as part of a work project, etc) then it is important that an
appropriate representative of the employer understand that the code
will be contributed under the GPL license. The arrangement should be
cleared with an authorized supervisor/manager, etc.

The code should be developed by the contributor, or the code should be
from a source which can be rightfully contributed such as from the
public domain, or from an open source project under a compatible
license.

All unusual situations need to be discussed and/or documented.

Committers should adhere to the following guidelines, and may be
personally legally liable for improperly contributing code to the
source repository:

* Make sure the contributor (and possibly employer) is aware of the
 contribution terms.
* Code coming from a source other than the contributor (such as
 adapted from another project) should be clearly marked as to the
 original source, copyright holders, license terms and so forth. This
 information can be in the file headers, but should also be added to
 the project licensing file if not exactly matching normal project
 licensing (grass/COPYRIGHT.txt).
* Existing copyright headers and license text should never be stripped
 from a file. If a copyright holder wishes to give up copyright they
 must do so in writing to the GRASS [PSC](https://trac.osgeo.org/grass/wiki/PSC) before copyright messages
 are removed. If license terms are changed, it has to be by agreement
 (written in email is ok) of the copyright holders.
* When substantial contributions are added to a file (such as
 substantial patches) the author/contributor should be added to the
 list of copyright holders for the file in the file header.
* If there is uncertainty about whether a change is proper to
 contribute to the code base, please seek more information from the
 Project Steering Committee, other GRASS developers or the OSGeo
 foundation legal counsel.

Questions regarding GRASS GIS should be directed to the
GRASS Development Team at the following address:

Internet: <http://grass.osgeo.org/home/contact-us/>

