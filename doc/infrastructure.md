# How the GRASS GIS Webserver and related infrastructure works

<<<<<<< HEAD
Author: Markus Neteler
Last update: Dec 2023
=======
written by M. Neteler
Last changed: July 2020

Related Wiki documents:

* <https://grass.osgeo.org/wiki/GRASS_Migration_to_OSGeo> (historical document)
<<<<<<< HEAD

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
## GRASS GIS Source code repository

Maintainer: Markus Neteler, Martin Landa, OSGeo-SAC, <http://wiki.osgeo.org/wiki/SAC>

Important update April 2019: The source code is now managed on GitHub (rather
than in SVN).

The GitHub repositories are:

<<<<<<< HEAD
<<<<<<< HEAD
- GRASS GIS core (7+): <https://github.com/OSGeo/grass>
- GRASS GIS legacy (3.x-6.x): <https://github.com/OSGeo/grass-legacy>
- GRASS GIS Add-ons: <https://github.com/OSGeo/grass-addons>
- GRASS GIS promotional material: <https://github.com/OSGeo/grass-promo>
- GRASS GIS Website (Hugo site): <https://github.com/OSGeo/grass-website>
- Github mirror at OSGeo: <https://git.osgeo.org/gitea/grass_gis/grass>

Git usage:

- [CONTRIBUTING.md file](../CONTRIBUTING.md)
- <https://trac.osgeo.org/grass/wiki/HowToGit>

Issues:

- <https://github.com/OSGeo/grass/issues>
- trac instance: <https://trac.osgeo.org/grass> (old bugs only)

Statistics:

- <https://github.com/OSGeo/grass/pulse>
=======
* GRASS GIS core (7.x): <https://github.com/OSGeo/grass>
* GRASS GIS legacy (3.x-6.x): <https://github.com/OSGeo/grass-legacy>
* GRASS GIS Add-ons: <https://github.com/OSGeo/grass-addons>
* GRASS GIS promotional material: <https://github.com/OSGeo/grass-promo>
* GRASS GIS Website (hugo site): <https://github.com/OSGeo/grass-website>
* Github mirror at OSGeo: <https://git.osgeo.org/gitea/grass_gis/grass>

Git usage:

* [CONTRIBUTING.md file](../CONTRIBUTING.md)
* <https://trac.osgeo.org/grass/wiki/HowToGit>

Issues:

=======
* GRASS GIS core (7.x): <https://github.com/OSGeo/grass>
* GRASS GIS legacy (3.x-6.x): <https://github.com/OSGeo/grass-legacy>
* GRASS GIS Add-ons: <https://github.com/OSGeo/grass-addons>
* GRASS GIS promotional material: <https://github.com/OSGeo/grass-promo>
* GRASS GIS Website (hugo site): <https://github.com/OSGeo/grass-website>
* Github mirror at OSGeo: <https://git.osgeo.org/gitea/grass_gis/grass>

Git usage:

* [CONTRIBUTING.md file](../CONTRIBUTING.md)
* <https://trac.osgeo.org/grass/wiki/HowToGit>

Issues:

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* <https://github.com/OSGeo/grass/issues>
* old trac instance: <https://trac.osgeo.org/grass>

Trac related notes:

<<<<<<< HEAD
* For easier linking in the Trac Wiki, some macro definitions are used for manual
  page refs (G7:modulename)
=======
<<<<<<< HEAD
* For easier linking in the Trac Wiki, some macro definitions are used for manual page refs (G7:modulename)
=======
* For easier linking in the Trac Wiki, some macro definitions are used for manual
  page refs (G7:modulename)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
  * <https://trac.osgeo.org/grass/wiki/InterMapTxt>
* ZIP file download support in trac (was needed for g.extension) * on trac.osgeo.org:

```text
/var/www/trac/grass/conf/trac.ini
[browser]
downloadable_paths = /grass-addons/grass7/*/*,/sandbox/*/*
```

Statistics:

* <https://github.com/OSGeo/grass/pulse>
* <https://trac.osgeo.org/grass/stats/code>
<<<<<<< HEAD

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
## GRASS Web server

Maintainer: M. Neteler

<<<<<<< HEAD
<<<<<<< HEAD
- <https://grass.osgeo.org>

  - osgeo7-grass: LXD container on osgeo7 (<https://wiki.osgeo.org/wiki/SAC_Service_Status#osgeo_7>)
    - OS: Debian Buster
    - Apache Server with Hugo (<https://github.com/OSGeo/grass-website>)
  - for migration details (7/2020), see <https://github.com/OSGeo/grass-website/issues/180>
  - ssh login: via jumphost hop.osgeo8.osgeo.org
  - deployment via cronjob: <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/>

- <https://old.grass.osgeo.org> (CMSMS, replaced in 2020 by above Hugo based solution)

  - Shared virtual OSGeo machine (osgeo6) hosted at Oregon State University
    Open Source Lab server: osgeo6.osgeo.osuosl.org)
  - Login: via OSGeo LDAP, there is a "grass" LDAP group
  - Software:
    - OS: Debian Wheezy
    - Apache Server with PHP
  - Login: via OSGeo LDAP, there is a "grass" LDAP group

<<<<<<< HEAD
<<<<<<< HEAD
- Backups:
=======
=======
>>>>>>> 227cbcebbf (Programmer's manual: update GRASS GIS arch drawing (#1610))
* GRASS programmer's manual (https://grass.osgeo.org/programming8/)
    * HTML: cronjob run Wednesday morning California time
    * HTML: cronjob run Saturday morning California time
    * disabled: PDF: cronjob run Saturday morning California time
>>>>>>> 73a1a8ce38 (Programmer's manual: update GRASS GIS arch drawing (#1610))

  - osgeo7-grass: container on osgeo8 is backup'ed, see <http://wiki.osgeo.org/wiki/SAC:Backups>

- Mirrors:

  - rsync, see <https://grass.osgeo.org/contribute/> --> Mirror
  - mirror list, see <https://grass.osgeo.org/about/mirrors/>

- RSS feed: offered by Hugo at <https://grass.osgeo.org/index.xml>, used at <https://planet.osgeo.org>

- Weekly software snapshots (generated Saturday morning Portland (OR), US time):

  - Source code tarball of git (GitHub) <https://github.com/OSGeo/grass>
  - Linux binary snapshot is compiled on osgeo7-grass
    - GRASS is compiled with GDAL, PROJ, SQLite, MySQL, PostgreSQL, FFTW, C++ support,
      see <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/>
    - binary tar.gz and manuals are moved into Web space

- GRASS user manual HTML:

  - generated during compilation of weekly Linux binary snapshot on osgeo7-grass

- GRASS addons manual HTML:

  - generated during compilation of weekly Linux binary snapshot on osgeo7-grass

- GRASS programmer's manual (<https://grass.osgeo.org/programming8/>)

  - HTML: cronjob run Wednesday morning Portland (OR), US time
  - HTML: cronjob run Saturday morning Portland (OR), US time
  - disabled: PDF: cronjob run Saturday morning Portland (OR), US time

- Mailman mailing lists + automated greylisting (at lists.osgeo.org since 11/2007)

  - Mailman is doing the job, only registered users can post
  - messages from unsubscribed people is auto-discarded without notification
  - the open "weblist" operates instead like this:
    - User -> grass-web at lists osgeo.org -> greylisting -> Mailman
    - for greylisting, see <http://postgrey.schweikert.ch/>

- Backup of mailing lists (mbox files)

  - nightly backup at OSGeo.org, bacula

- Web statistics
  - Matomo: <https://2022.foss4g.org/matomo/> (not publicly accessible;
    access: Markus Neteler)
  - Selected stats: <http://wiki.osgeo.org/wiki/Project_Stats>
=======
* <https://grass.osgeo.org>
  * osgeo8-grass: LXD container on osgeo8 (<https://wiki.osgeo.org/wiki/SAC_Service_Status#osgeo_8>)
    * OS: Debian Buster
    * Apache Server with hugo (<https://github.com/OSGeo/grass-website>)
  * for migration details (7/2020), see <https://github.com/OSGeo/grass-website/issues/180>
  * ssh login: via jumphost hop.osgeo8.osgeo.org
  * deployment via cronjob: <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/>
* <https://old.grass.osgeo.org> (CMSMS, replaced in 2020 by above hugo based solution)
  * Shared virtual OSGeo machine (osgeo6) hosted at Oregon State University
    Open Source Lab server: osgeo6.osgeo.osuosl.org)
  * Login: via OSGeo LDAP, there is a "grass" LDAP group
  * Software:
    * OS: Debian Wheezy
    * Apache Server with PHP
  * Login: via OSGeo LDAP, there is a "grass" LDAP group
* Backups:
  * osgeo8-grass: container on osgeo8 is backup'ed, see <http://wiki.osgeo.org/wiki/SAC:Backups>
* Mirrors:
  * rsync, see <https://grass.osgeo.org/contribute/>  --> Mirror
  * mirror list, see <https://grass.osgeo.org/about/mirrors/>
* RSS feed: offered by hugo at <https://grass.osgeo.org/index.xml>, used at <https://planet.osgeo.org>

* Weekly software snapshots (generated Saturday morning Portland (OR), US time):
  * Source code tarball of git (GitHub) <https://github.com/OSGeo/grass>
  * Linux binary snapshot is compiled on osgeo8-grass
    * GRASS is compiled with GDAL, PROJ, SQLite, MySQL, PostgreSQL, FFTW, C++ support
    * binary tar.gz and manuals are moved into Web space

* GRASS user manual HTML:
  * generated during compilation of weekly Linux binary snapshot on osgeo8-grass

* GRASS addons manual HTML:
  * generated during compilation of weekly Linux binary snapshot on osgeo8-grass

* GRASS programmer's manual (<https://grass.osgeo.org/programming8/>)
  * HTML: cronjob run Wednesday morning Portland (OR), US time
  * HTML: cronjob run Saturday morning Portland (OR), US time
  * disabled: PDF: cronjob run Saturday morning Portland (OR), US time

=======
* <https://grass.osgeo.org>
  * osgeo8-grass: LXD container on osgeo8 (<https://wiki.osgeo.org/wiki/SAC_Service_Status#osgeo_8>)
    * OS: Debian Buster
    * Apache Server with hugo (<https://github.com/OSGeo/grass-website>)
  * for migration details (7/2020), see <https://github.com/OSGeo/grass-website/issues/180>
  * ssh login: via jumphost hop.osgeo8.osgeo.org
  * deployment via cronjob: <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/>
* <https://old.grass.osgeo.org> (CMSMS, replaced in 2020 by above hugo based solution)
<<<<<<< HEAD
  * Shared virtual OSGeo machine (osgeo6) hosted at Oregon State University Open Source Lab
  server: osgeo6.osgeo.osuosl.org)
=======
  * Shared virtual OSGeo machine (osgeo6) hosted at Oregon State University
    Open Source Lab server: osgeo6.osgeo.osuosl.org)
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
  * Login: via OSGeo LDAP, there is a "grass" LDAP group
  * Software:
    * OS: Debian Wheezy
    * Apache Server with PHP
  * Login: via OSGeo LDAP, there is a "grass" LDAP group
* Backups:
  * osgeo8-grass: container on osgeo8 is backup'ed, see <http://wiki.osgeo.org/wiki/SAC:Backups>
* Mirrors:
  * rsync, see <https://grass.osgeo.org/contribute/>  --> Mirror
  * mirror list, see <https://grass.osgeo.org/about/mirrors/>
* RSS feed: offered by hugo at <https://grass.osgeo.org/index.xml>, used at <https://planet.osgeo.org>

* Weekly software snapshots (generated Saturday morning Portland (OR), US time):
  * Source code tarball of git (GitHub) <https://github.com/OSGeo/grass>
  * Linux binary snapshot is compiled on osgeo8-grass
    * GRASS is compiled with GDAL, PROJ, SQLite, MySQL, PostgreSQL, FFTW, C++ support
    * binary tar.gz and manuals are moved into Web space

* GRASS user manual HTML:
  * generated during compilation of weekly Linux binary snapshot on osgeo8-grass

* GRASS addons manual HTML:
  * generated during compilation of weekly Linux binary snapshot on osgeo8-grass

* GRASS programmer's manual (<https://grass.osgeo.org/programming8/>)
  * HTML: cronjob run Wednesday morning Portland (OR), US time
  * HTML: cronjob run Saturday morning Portland (OR), US time
  * disabled: PDF: cronjob run Saturday morning Portland (OR), US time

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* i18N translation statistics (<https://grass.osgeo.org/development/translations/#statistics>)
<<<<<<< HEAD
  * generated during compilation of Linux binary snapshot, stats of
    `(cd locale; make)` are extracted into text file
=======
<<<<<<< HEAD
  * generated during compilation of Linux binary snapshot, stats of `(cd locale; make)` are extracted into text file
=======
  * generated during compilation of Linux binary snapshot, stats of
    `(cd locale; make)` are extracted into text file
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
  * text file parsed by PHP page and shown as table
  * GRASS GIS version is coded in devel/i18n_stats.inc
  * for Transifex integration, see below

* Mailman mailing lists + greylisting (at lists.osgeo.org since 11/2007)
  * Mailman is doing the job, only registered users can post
  * messages from unsubscribed people is auto-discarded without notification
  * the open "weblist" operates instead like this:
    * User -> grass-web at lists osgeo.org -> greylisting -> Mailman

* Backup of mailing lists (mbox files)
  * nightly backup at OSGeo.org, bacula

* Web statistics
  * See URL at <http://wiki.osgeo.org/wiki/Project_Stats>
    * cronjob script: /osgeo/scripts/update_logs.sh
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

Summary: The system should run almost autonomously.

## WinGRASS maintenance scripts

<<<<<<< HEAD
<<<<<<< HEAD
See <https://github.com/landam/wingrass-maintenance-scripts>
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* <https://github.com/landam/wingrass-maintenance-scripts>
<<<<<<< HEAD

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
## GRASS Mailing lists

Maintainer: Markus Neteler

Available lists:

<<<<<<< HEAD
<<<<<<< HEAD
- at OSGeo.org (<https://lists.osgeo.org/mailman/listinfo>):
  - grass-abm: Integration of GRASS with JAVA based agent based modeling
    (ABM)
  - grass-announce: GRASS announcements
  - grass-commit: Mailing list to distribute GRASS Github commits
  - grass-dev: GRASS GIS Development mailing list
  - grass-es: La lista de correo de GRASS GIS en español
  - grass-psc: GRASS-PSC: GRASS Project Steering Committee
  - grass-stats: GRASS and statistical software
  - grass-translations: Translation of GRASS (i18N)
  - grass-user: GRASS user list
  - grass-web: GRASS website mailing list

Notes:
=======
* at OSGeo.org (<https://lists.osgeo.org/mailman/listinfo>):
  * grass-abm           Integration of GRASS with JAVA based agent based modeling
                        (ABM)
  * grass-announce      GRASS announcements
  * grass-commit        Mailing list to distribute GRASS-CVS commits
  * grass-dev           GRASS GIS Development mailing list
  * grass-es            La lista de correo de GRASS GIS en español
  * grass-psc           GRASS-PSC: GRASS Project Steering Committee
  * grass-stats         GRASS and statistical software
  * grass-translations  Translation of GRASS (i18N)
  * grass-user          GRASS user list
  * grass-web           GRASS website mailing list

=======
* at OSGeo.org (<https://lists.osgeo.org/mailman/listinfo>):
<<<<<<< HEAD
   grass-abm            Integration of GRASS with JAVA based agent based modeling (ABM)
   grass-announce GRASS announcements
   grass-commit         Mailing list to distribute GRASS-CVS commits
   grass-dev            GRASS GIS Development mailing list
   grass-es             La lista de correo de GRASS GIS en español
   grass-psc            GRASS-PSC: GRASS Project Steering Committee
   grass-stats          GRASS and statistical software
   grass-translations   Translation of GRASS (i18N)
   grass-user           GRASS user list
   grass-web            GRASS website mailing list

>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* OLD, UNUSED: at FBK-irst (<http://grass.fbk.eu/mailman/admin/>):
  * grass-commit-addons  Mailing list to distribute GRASS Addons-SVN commits
  * grass-gui            GRASSGUI mailing list
  * grass-qa             GRASS Quality Assessment and monitoring list
  * grass-windows        winGRASS * Using GRASS on MS-Windows systems mailing list

Notes:

* grass-announce:
  * moderated by M. Neteler
  * has monthly password reminder disabled to avoid leakage into publicly
    archived lists
* grass-commit is receiving posts from the GRASS SVN at osgeo.org. Not open for
  other postings, they will be trashed automatically
* grass-web is an open list (posting without subscription possible) with (Google)
  spam filter
  * moderated by M. Neteler to avoid spam
<<<<<<< HEAD
* OLD, UNUSED: grass-qa is receiving posts from the GRASS Quality Control System at Ecole Polytechnique de Montreal, Canada. Not open for other postings.
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

- grass-announce:
  - moderated by M. Neteler
  - monthly password reminder is disabled to avoid leakage into publicly
    archived lists
- grass-commit is receiving posts from the GRASS Github. Not open for
  other postings, they will be trashed automatically
- grass-web is an open list (posting without subscription possible with
  moderation), moderated by M. Neteler to avoid spam
=======
* OLD, UNUSED: grass-qa is receiving posts from the GRASS Quality Control System
  at Ecole Polytechnique de Montreal, Canada. Not open for other postings.
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
=======
  * grass-abm           Integration of GRASS with JAVA based agent based modeling
                        (ABM)
  * grass-announce      GRASS announcements
  * grass-commit        Mailing list to distribute GRASS-CVS commits
  * grass-dev           GRASS GIS Development mailing list
  * grass-es            La lista de correo de GRASS GIS en español
  * grass-psc           GRASS-PSC: GRASS Project Steering Committee
  * grass-stats         GRASS and statistical software
  * grass-translations  Translation of GRASS (i18N)
  * grass-user          GRASS user list
  * grass-web           GRASS website mailing list

* OLD, UNUSED: at FBK-irst (<http://grass.fbk.eu/mailman/admin/>):
  * grass-commit-addons  Mailing list to distribute GRASS Addons-SVN commits
  * grass-gui            GRASSGUI mailing list
  * grass-qa             GRASS Quality Assessment and monitoring list
  * grass-windows        winGRASS * Using GRASS on MS-Windows systems mailing list

Notes:
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))

* grass-announce:
  * moderated by M. Neteler
  * has monthly password reminder disabled to avoid leakage into publicly
    archived lists
* grass-commit is receiving posts from the GRASS SVN at osgeo.org. Not open for
  other postings, they will be trashed automatically
* grass-web is an open list (posting without subscription possible) with (Google)
  spam filter
  * moderated by M. Neteler to avoid spam
* OLD, UNUSED: grass-qa is receiving posts from the GRASS Quality Control System
  at Ecole Polytechnique de Montreal, Canada. Not open for other postings.
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

## GRASS Wiki

Maintainer: Martin Landa, Markus Neteler

<<<<<<< HEAD
<<<<<<< HEAD
- <https://grasswiki.osgeo.org>
- Mediawiki software
- requires registration to keep spammers out
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* <https://grasswiki.osgeo.org>
* Mediawiki
* mirrored at CZ Tech University
* requires registration to keep spammers out
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

<<<<<<< HEAD
<<<<<<< HEAD
Summary: The system should run almost autonomous. An eye must be be kept
on people trying to spam the site. Several layers of registration protection
are in place due to excessive spam.

Macros for manual pages (src, cmd, API, ...):
<<<<<<< HEAD
<<<<<<< HEAD

- <https://grasswiki.osgeo.org/wiki/Category:Templates>

## GRASS IRC

Channel: irc://irc.libera.chat/grass
Web based client: See <https://grasswiki.osgeo.org/wiki/IRC>

- channel owner: Alessandro Frigeri ("geoalf")
- quasi guru level: Markus Neteler ("markusN")
- original (freenode) operators:
  - Jachym ("jachym")
  - Luca ("doktoreas")
  - Soeren ("huhabla")
  - Brad ("bdouglas")
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
Summary: The system should run almost autonomous. An eye must be be kept on people
trying to spam the site

Macros for manual pages (src, cmd, API, ...):

>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
Summary: The system should run almost autonomous. An eye must be be kept on people
trying to spam the site

Macros for manual pages (src, cmd, API, ...):
<<<<<<< HEAD
=======

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
* <https://grasswiki.osgeo.org/wiki/Category:Templates>

## GRASS IRC

Channel: irc://irc.freenode.net/grass
Web based client: See <https://grasswiki.osgeo.org/wiki/IRC>

* channel owner: Alessandro Frigeri < afrigeri unipg.it > ("geoalf")
* quasi guru level: Markus Neteler ("markusN")
* further operators:
  * Jachym ("jachym")
  * Luca ("doktoreas")
  * Soeren ("huhabla")
  * Brad ("bdouglas")
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

## GRASS Bugtracker

Current bugtracker (Jan 2020 - today):
<<<<<<< HEAD
<<<<<<< HEAD

- <https://github.com/OSGeo/grass/issues>

Old bugtrackers: see <https://grasswiki.osgeo.org/wiki/Bug_tracking>

## GRASS GIS Addons
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

* <https://github.com/OSGeo/grass/issues>

Old bugtracker (Jan 2008 - Jan 2020):

* <https://trac.osgeo.org/grass/report>
* posted new bugs and comments to grass-dev list
* Settings:

Old tracsvn (OSGeo server) (Dec 2007 * Mai 2019)
<<<<<<< HEAD

=======
<<<<<<< HEAD
=======

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
```text
/var/www/trac/env/grass/conf/trac.ini
  downloadable_paths = /grass-addons/grass7/*/*,/sandbox/*/*
  path = /var/www/grass/htdocs
  link = <https://grass.osgeo.org/>
  src = site/grasslogo_vector_small.png
  smtp_always_cc = grass-dev@lists.osgeo.org
  smtp_replyto = grass-dev@lists.osgeo.org
  url = <https://grass.osgeo.org>
  .dir = /var/www/svn/repos/grass
  base_url = <https://trac.osgeo.org/grass/>
  database = postgres://postgres@/trac_grass
```

Very old bugtracker (Jan 2007 * Dec 2008):

* <http://wald.intevation.org/tracker/?group_id=21>
* gforce, sponsored by Intevation GmbH, Germany
* spamassasin spamfilter locally, bogofilter at grass-dev list
* needs `noreply*wald.intevation.org` to be enabled as alias in Mailman

Very very old bugtracker (Dec 2000 * Dec 2006):
<<<<<<< HEAD

* <https://intevation.de/rt/webrt?q_queue=grass>
* webRT, sponsored by Intevation GmbH, Germany
* spamassasin spamfilter locally, bogofilter at grass-dev list
* reports are directly sent to GRASS Developers mailing list for notification
* TODO: migrate to trac
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))

<<<<<<< HEAD
=======
* <https://intevation.de/rt/webrt?q_queue=grass>
* webRT, sponsored by Intevation GmbH, Germany
* spamassasin spamfilter locally, bogofilter at grass-dev list
* reports are directly sent to GRASS Developers mailing list for notification
* TODO: migrate to trac

>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
## GRASS Addons
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

Maintainer: Martin Landa and Markus Neteler

Details:

<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
- Windows-addons: grass-addons/utils/addons/README.txt
- Addon manual pages cronjob: <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd>
- Rendered manuals: <https://grass.osgeo.org/grass8/manuals/addons/>
=======
- Windows-addons: grass-addons/tools/addons/README.txt
- Addon manual pages cronjob: https://github.com/OSGeo/grass-addons/tree/master/tools/cronjobs_osgeo_lxd
- Rendered manuals: https://grass.osgeo.org/grass8/manuals/addons/
<<<<<<< HEAD
>>>>>>> 73a1a8ce38 (Programmer's manual: update GRASS GIS arch drawing (#1610))
=======
>>>>>>> 227cbcebbf (Programmer's manual: update GRASS GIS arch drawing (#1610))
=======
* Windows-addons: grass-addons/utils/addons/README.txt
* Addon manual pages cronjob: <https://github.com/OSGeo/grass-addons/tree/master/utils/cronjobs_osgeo_lxd>
* Rendered manuals: <https://grass.osgeo.org/grass8/manuals/addons/>
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
* Windows-addons: grass-addons/utils/addons/README.txt
* Addon manual pages cronjob: <https://github.com/OSGeo/grass-addons/tree/master/utils/cronjobs_osgeo_lxd>
* Rendered manuals: <https://grass.osgeo.org/grass8/manuals/addons/>
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

The redirect to the latest grassX directory is defined on grass.osgeo.org:
/etc/apache2/includes/grass.osgeo.org.inc

Procedure building of binaries (Windows):
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD

- Addons module are compiled on winGRASS build server, at the CTU in Prague) and
  publishing their manual pages on publishing server, i.e. grass.osgeo.org.
- A new compilation is triggered every time a commit is done in the Addons repo.
- Logs:
  - Linux log files: <https://grass.osgeo.org/addons/grass8/logs> (compiled on
    `grasslxd` on `osgeo7`)
  - Windows log files: <http://wingrass.fsv.cvut.cz/grass8X/addons/grass-XXXdev/logs/>

Procedure of granting write access to Addons repo:

- Request procedure: <https://github.com/OSGeo/grass/blob/main/doc/development/submitting/submitting.md>
- Adding OSGeo-ID: <https://www.osgeo.org/community/getting-started-osgeo/osgeo_userid/>
- Adding contributor: <https://trac.osgeo.org/grass/browser/grass-addons/contributors.csv>
  (via git commit)
- Confirm request in grass-psc and give instructions concerning code style etc
  (see archive for examples)

XML file for g.extension: <https://grass.osgeo.org/addons/grass8/modules.xml>

- generated in grass-addons/utils/addons/grass-addons-publish.sh
=======
* Addons module are compiled on build server, currently at the CTU in Prague) and publishing their manual pages on publishing server, i.e. grass.osgeo.org.
=======

* Addons module are compiled on build server, currently at the CTU in Prague) and
  publishing their manual pages on publishing server, i.e. grass.osgeo.org.
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
* A new compilation is triggered everytime when a commit is done in the Addons-SVN.
* Logs:
  * Linux log files:   <https://grass.osgeo.org/addons/grass7/logs> (compiled on
    `grasslxd` on `osgeo7`)
=======

* Addons module are compiled on build server, currently at the CTU in Prague) and
  publishing their manual pages on publishing server, i.e. grass.osgeo.org.
* A new compilation is triggered everytime when a commit is done in the Addons-SVN.
* Logs:
<<<<<<< HEAD
  * Linux log files:   <https://grass.osgeo.org/addons/grass7/logs> (compiled on `grasslxd` on `osgeo7`)
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
  * Windows log files: <http://wingrass.fsv.cvut.cz/grass78/x86_64/addons/latest/logs/>

Procedure of granting write access to Addons repo:

* Request procedure: <https://trac.osgeo.org/grass/wiki/HowToContribute#WriteaccesstotheGRASS-Addons-SVNrepository>
* Adding OSGeo-ID:   <https://www.osgeo.org/cgi-bin/auth/ldap_group.py?group=grass_addons>
* Adding contributor: <https://trac.osgeo.org/grass/browser/grass-addons/contributors.csv>
  (via SVN commit)
* Confirm request in grass-psc and give instructions concerning code style etc
  (see archive for examples)

XML file for g.extension: <https://grass.osgeo.org/addons/grass7/modules.xml>
<<<<<<< HEAD

=======
=======
  * Linux log files:   <https://grass.osgeo.org/addons/grass7/logs> (compiled on
    `grasslxd` on `osgeo7`)
  * Windows log files: <http://wingrass.fsv.cvut.cz/grass78/x86_64/addons/latest/logs/>

Procedure of granting write access to Addons repo:

* Request procedure: <https://trac.osgeo.org/grass/wiki/HowToContribute#WriteaccesstotheGRASS-Addons-SVNrepository>
* Adding OSGeo-ID:   <https://www.osgeo.org/cgi-bin/auth/ldap_group.py?group=grass_addons>
* Adding contributor: <https://trac.osgeo.org/grass/browser/grass-addons/contributors.csv>
  (via SVN commit)
* Confirm request in grass-psc and give instructions concerning code style etc
  (see archive for examples)

XML file for g.extension: <https://grass.osgeo.org/addons/grass7/modules.xml>

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
* generated in grass-addons/utils/addons/grass-addons-publish.sh
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

## GRASS Travis CI

Maintainer: Martin Landa

<<<<<<< HEAD
<<<<<<< HEAD
- <https://app.travis-ci.com/github/OSGeo/grass>
- <https://github.com/OSGeo/grass/tree/main/.travis/>
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* <https://travis-ci.org/GRASS-GIS>
* <https://github.com/OSGeo/grass>
* OLD: <https://github.com/GRASS-GIS/grass-ci>
* <https://github.com/OSGeo/grass-addons/tree/master/utils/grass-ci/>

Travis CI control files:
 trunk/.travis/
   linux.before_install.sh
   linux.install.sh
   linux.script.sh

Maintenance script:
<<<<<<< HEAD

=======
<<<<<<< HEAD
=======

>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
* <https://github.com/OSGeo/grass-addons/tree/master/utils/grass-ci/grass-ci.sh>

The github update is run as a cronjob on server "geo102" (CTU, CZ).

<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
## GRASS CI: GitHub Actions

Started in Apr. 2020

Maintainer: Vaclav Petras

<<<<<<< HEAD
<<<<<<< HEAD
- <https://github.com/OSGeo/grass/actions>
- Details: <https://github.com/OSGeo/grass/pull/525>
- CI workflow with:
  - A build job which is not parallelized and is meant for clear & relatively fast
    check of compilation and building in general. (Duplicating what is running
    on Travis)
  - A test job which of course needs to build, but the main focus is to run tests,
    so the compilation is parallelized (depending on nproc) and thus potentially
    less readable. This runs the whole test suite. (You need to run it locally to
    see the actual error, but you can see which tests are failing.)
- Static code analysis/Code quality check using Flake8 with separate checks for
  python/grass, gui/wxpython, scripts and temporal directories.
  - Configurations ignore different lists of Flake8 errors. The idea is to reduce
    that to minimum.
  - Code in testsuite directories is also ignored for now, but should not be in
    the future.

Helper files placed in .github/workflows/
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* <https://github.com/OSGeo/grass/actions>
* Details: <https://github.com/OSGeo/grass/pull/525>

* CI workflow with:
<<<<<<< HEAD
  * A build job which is not parallelized and is meant for clear & relatively fast check of compilation and building in general. (Duplicating what is running on Travis)
  * A test job which of course needs to build, but the main focus is to run tests, so the compilation is parallelized (depending on nproc) and thus potentially less readable. This runs the whole test suite. (You need to run it locally to see the actual error, but you can see which tests are failing.)
* Static code analysis/Code quality check using Flake8 with separate checks for python/grass, gui/wxpython, scripts and temporal directories.
  * Configurations ignore different lists of Flake8 errors. The idea is to reduce that to minimum.
  * Code in testsuite directories is also ignored for now, but should not be in the future.
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
=======
  * A build job which is not parallelized and is meant for clear & relatively fast
    check of compilation and building in general. (Duplicating what is running
    on Travis)
  * A test job which of course needs to build, but the main focus is to run tests,
    so the compilation is parallelized (depending on nproc) and thus potentially
    less readable. This runs the whole test suite. (You need to run it locally to
    see the actual error, but you can see which tests are failing.)
* Static code analysis/Code quality check using Flake8 with separate checks for
  python/grass, gui/wxpython, scripts and temporal directories.
  * Configurations ignore different lists of Flake8 errors. The idea is to reduce
    that to minimum.
  * Code in testsuite directories is also ignored for now, but should not be in
    the future.
<<<<<<< HEAD
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

## GRASS docker images

<<<<<<< HEAD
<<<<<<< HEAD
Maintainer: Carmen Tawalika, Vaclav Petras + OSGeo-SAC

Docker images are created with a GitHub action. Subsequently, login is done
to DockerHub using `docker/login-action` with username and password through
CI secrets and the images pushed to Docker hub at:

- <https://hub.docker.com/r/osgeo/grass-gis/>

User settings:

- The GRASS GIS CI user at Docker hub is "grassgis" (joined June 3, 2023),
  see also <https://hub.docker.com/u/grassgis>
- Docker Hub access token are managed via <grass-ci-admin@osgeo.org>.
- The OSGeo Org membership is managed at <https://hub.docker.com/orgs>
  through OSGeo-SAC

Helper files placed in .github/workflows/

## GRASS Zenodo.org repository: citable source code with DOI

Zenodo page with DOI (for all versions, shows latest release on top):
[10.5281/zenodo.5176030](https://doi.org/10.5281/zenodo.5176030)

**GitHub - Zenodo Integration**: The settings are accessible with any
GitHub account which has write access to the GRASS GIS GitHub repo and
they are managed here:

<https://zenodo.org/account/settings/github/repository/OSGeo/grass>

In the settings, the GRASS GIS GitHub repository needs to be enabled.

Upcoming releases should automatically show up and get a DOI
assigned.

Explanations: <https://grasswiki.osgeo.org/wiki/GitHub-Zenodo_linkage>

=======
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
## GRASS Coverity Scan

Maintainer: Markus Neteler

<<<<<<< HEAD
<<<<<<< HEAD
Coverity Scan is a service to find security issues. At time the service
is used only occasionally.
=======
* <https://scan.coverity.com/projects/grass?tab=overview>
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
* <https://scan.coverity.com/projects/grass?tab=overview>
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

<<<<<<< HEAD
- <https://scan.coverity.com/projects/grass?tab=overview>
=======
<<<<<<< HEAD

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

## User message translation management (i18N)
=======
## Transifex translation management
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

Messages are extracted with `gettext` message macros.

<<<<<<< HEAD
<<<<<<< HEAD
Translations may be done using the OSGeo Weblate platform:
=======
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
* Dashboard: <https://www.transifex.com/grass-gis/>
* Auto-update URL to fetch files:
  * <https://www.transifex.com/grass-gis/grass7/content/>
    * Menu: Resources
      * Use: "Auto update resources" button
* Weblate: <https://weblate.osgeo.org/>
<<<<<<< HEAD
<<<<<<< HEAD
=======

<<<<<<< HEAD
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))
=======

=======
>>>>>>> 7409ab6716 (r.horizon manual - fix typo (#2794))
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))

>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
## OLD: GRASS Quality Control
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))

- Weblate: <https://weblate.osgeo.org/>
  - GRASS GIS Weblate server: <https://weblate.osgeo.org/projects/grass-gis/>

Anyone with OSGeo-LDAP access can work on the translations.

<<<<<<< HEAD
For technical background and access rights of the Weblate installation,
see <https://wiki.osgeo.org/wiki/SAC:Weblate>.

### How Weblate works

When a developer makes a GRASS GIS repo commit on GitHub, GitHub then calls
the webhook on Weblate which triggers a refresh of Weblate's git copy of the
GRASS GIS repo.

For pushing translations back to GitHub, there is a setting in Weblate for that
which defaults to 24 hrs (accumulates translations over a day). Then a pull
request with the translations will be opened in the GRASS GIS GitHub repo.

### Weblate troubleshooting

In case the Weblate's git copy of the GRASS GIS repo does not update due to
a conflict or whatever reason:

Log into Weblate (requires administrator rights) and switch to
<https://weblate.osgeo.org/projects/grass-gis/#repository>. Therein click on
"Manage" -> "Repository Maintenance", choose the "Update" button,
"Update with merge without fast-forward". If successful, this will create
another pull request in the GRASS GIS repo (trigger with "Push" button).

## Related Wiki documents

- <https://grass.osgeo.org/wiki/GRASS_Migration_to_OSGeo> (historical document)
=======
* offline.
* <http://web.soccerlab.polymtl.ca/grass-evolution/grass-browsers/grass-index-en.html>
   was implemented and sponsored by Ecole Polytechnique de Montreal, Canada
* Realtime analysis has been sent to: <http://lists.osgeo.org/mailman/listinfo/grass-qa>

Further notification/functionality test systems:

* posts into #grass IRC channel
* posts into #osgeo-commits IRC channel
<<<<<<< HEAD
>>>>>>> 6cf60c76a4 (wxpyimgview: explicit conversion to int (#2704))
=======
>>>>>>> 8422103f4c (wxpyimgview: explicit conversion to int (#2704))

## Previous hosting sponsors

Since 2010 the GRASS GIS project is hosted at the Oregon State University Open
Source Lab (OSUOSL), Oregon, USA

Numerous institutions have sponsored the GRASS Project with Hardware/Bandwidth
(list of master site hosting):

<<<<<<< HEAD
<<<<<<< HEAD
- 1997-1999: Institut fuer Landschaftspflege und Naturschutz (ILN), Universitaet
  Hannover, Germany
- 1999-2001: Institut fuer Physische Geographie und Landschaftsoekologie,
  Universitaet Hannover, Germany
- 2001-2008: ITC-irst, Trento, Italy
- 2009-2010: Telascience.org at San Diego Supercomputer Center, California, USA
- 2010-today: Oregon State University | Open Source Lab, USA
=======
=======
>>>>>>> f130b43e6c (r.horizon manual - fix typo (#2794))
* 1997-1999: Institut fuer Landschaftspflege und Naturschutz (ILN), Universitaet
  Hannover, Germany
* 1999-2001: Institut fuer Physische Geographie und Landschaftsoekologie,
  Universitaet Hannover, Germany
* 2001-2008: ITC-irst, Trento, Italy
* 2009-2010: Telascience.org at San Diego Supercomputer Center, California, USA
* 2010-today: Oregon State University | Open Source Lab, USA
>>>>>>> 498a331298 (Fix missing function prototypes (#2727))
