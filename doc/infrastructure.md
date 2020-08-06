# How the GRASS GIS Webserver and related infrastructure works

written by M. Neteler
Last changed: July 2020

Related Wiki documents:

* https://grass.osgeo.org/wiki/GRASS_Migration_to_OSGeo (historical document)


## GRASS GIS Source code repository

Maintainer: Markus Neteler, Martin Landa, OSGeo-SAC, http://wiki.osgeo.org/wiki/SAC

Important update April 2019: The source code is now managed on GitHub (rather than in SVN).

The GitHub repositories are:

* GRASS GIS core (7.x): https://github.com/OSGeo/grass
* GRASS GIS legacy (3.x-6.x): https://github.com/OSGeo/grass-legacy
* GRASS GIS Add-ons: https://github.com/OSGeo/grass-addons
* GRASS GIS promotional material: https://github.com/OSGeo/grass-promo
* GRASS GIS Website (hugo site): https://github.com/OSGeo/grass-website
* Github mirror at OSGeo: https://git.osgeo.org/gitea/grass_gis/grass

Git usage:

* https://github.com/OSGeo/grass/blob/master/CONTRIBUTING.md
* https://trac.osgeo.org/grass/wiki/HowToGit

Issues:

* https://github.com/OSGeo/grass/issues
* old trac instance: https://trac.osgeo.org/grass

Trac related notes:

* For easier linking in the Trac Wiki, some macro definitions are used for manual page refs (G7:modulename)
    * https://trac.osgeo.org/grass/wiki/InterMapTxt
* ZIP file download support in trac (was needed for g.extension) * on trac.osgeo.org:

```
/var/www/trac/grass/conf/trac.ini
[browser]
downloadable_paths = /grass-addons/grass7/*/*,/sandbox/*/*
```

Statistics:

* https://github.com/OSGeo/grass/pulse
* https://trac.osgeo.org/grass/stats/code


## GRASS Web server

Maintainer: M. Neteler

* https://grass.osgeo.org
    * grasslxd: LXD container on osgeo7 (https://wiki.osgeo.org/wiki/SAC_Service_Status#osgeo7)
        * OS: Debian Buster
        * Apache Server with hugo
    * for migration details (7/2020), see https://github.com/OSGeo/grass-website/issues/180
    * ssh login: via osgeo jumphost
    * deployment via cronjob: https://github.com/OSGeo/grass-addons/tree/master/tools/cronjobs_osgeo_lxd
* https://old.grass.osgeo.org (CMSMS, replaced in 2020 by hugo based solution)
    * Shared virtual OSGeo machine (osgeo6) hosted at Oregon State University Open Source Lab
   (server: osgeo6.osgeo.osuosl.org)
    * Login: via OSGeo LDAP, there is a "grass" LDAP group
    * Software:
        * OS: Debian Wheezy
        * Apache Server with PHP
    * Login: via OSGeo LDAP, there is a "grass" LDAP group
* Backup:
    * grasslxd container on osgeo7 is backup'ed, see http://wiki.osgeo.org/wiki/SAC:Backups
    * Wiki backup, via rsync to http://josef.fsv.cvut.cz/WIKI/grass-osgeo/index.php/Main_Page
* Mirrors:
    * rsync, see https://grass.osgeo.org/contribute/
    * mirror list, see https://grass.osgeo.org/about/mirrors/
* RSS feed: offered by hugo at https://grass.osgeo.org/index.xml, used at https://planet.osgeo.org

* Weekly software snapshots (generated Saturday morning California time):
    * Source code tarball of git (GitHub) https://github.com/OSGeo/grass
    * Linux binary snapshot is compiled on grasslxd
        * GRASS is compiled with GDAL, PROJ, SQLite, MySQL, PostgreSQL, FFTW, C++ support
        * binary tar.gz and manuals are moved into Web space

* GRASS user manual HTML:
    * generated during compilation of weekly Linux binary snapshot on grasslxd

* GRASS addons manual HTML:
    * generated during compilation of weekly Linux binary snapshot on grasslxd

* GRASS programmer's manual (https://grass.osgeo.org/programming7/)
    * HTML: cronjob run Wednesday morning California time
    * HTML: cronjob run Saturday morning California time
    * disabled: PDF: cronjob run Saturday morning California time

* i18N translation statistics (https://grass.osgeo.org/development/translations/#statistics)
    * generated during compilation of Linux binary snapshot, stats of `(cd locale; make)` are extracted into text file
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
    * See URL at http://wiki.osgeo.org/wiki/Project_Stats
        * cronjob script: /osgeo/scripts/update_logs.sh

Summary: The system should run almost autonomously.


## WinGRASS maintenance scripts

* https://github.com/landam/wingrass-maintenance-scripts


## GRASS Mailing lists

Maintainer: Markus Neteler

Available lists:

* at OSGeo.org (https://lists.osgeo.org/mailman/listinfo):
   grass-abm 	 	Integration of GRASS with JAVA based agent based modeling (ABM)
   grass-announce GRASS announcements
   grass-commit 	Mailing list to distribute GRASS-CVS commits
   grass-dev 		GRASS GIS Development mailing list
   grass-es 		La lista de correo de GRASS GIS en español
   grass-psc 		GRASS-PSC: GRASS Project Steering Committee
   grass-stats 	GRASS and statistical software
   grass-translations 	Translation of GRASS (i18N)
   grass-user 		GRASS user list
   grass-web 		GRASS website mailing list

* OLD, UNUSED: at FBK-irst (http://grass.fbk.eu/mailman/admin/):
   grass-commit-addons  Mailing list to distribute GRASS Addons-SVN commits
   grass-gui 		GRASSGUI mailing list
   grass-qa 		GRASS Quality Assessment and monitoring list
   grass-windows 	winGRASS * Using GRASS on MS-Windows systems mailing list

Notes:
* grass-announce:
    * moderated by M. Neteler
    * has monthly password reminder disabled to avoid leakage into publicly archived lists
* grass-commit is receiving posts from the GRASS SVN at osgeo.org. Not open for other postings, they will be trashed automatically
* grass-web is an open list (posting without subscription possible) with (Google) spam filter
    * moderated by M. Neteler to avoid spam
* OLD, UNUSED: grass-qa is receiving posts from the GRASS Quality Control System at Ecole Polytechnique de Montreal, Canada. Not open for other postings.


## GRASS Wiki

Maintainer: Martin Landa, Markus Neteler

* https://grasswiki.osgeo.org
* Mediawiki
* mirrored at CZ Tech University
* requires registration to keep spammers out

Summary: The system should run almost autonomous. An eye must be be kept on people trying to spam the site

Macros for manual pages (src, cmd, API, ...):
* https://grasswiki.osgeo.org/wiki/Category:Templates

## GRASS IRC

Channel: irc://irc.freenode.net/grass
Web based client: See https://grasswiki.osgeo.org/wiki/IRC

* channel owner: Alessandro Frigeri <afrigeri unipg.it> ("geoalf")
* quasi guru level: Markus Neteler ("markusN")
* further operators:
   * Jachym ("jachym")
   * Luca ("doktoreas")
   * Soeren ("huhabla")
   * Brad ("bdouglas")

## GRASS Bugtracker

Current bugtracker (Jan 2020 - today):
  * https://github.com/OSGeo/grass/issues

Old bugtracker (Jan 2008 - Jan 2020):
  * https://trac.osgeo.org/grass/report
  * posted new bugs and comments to grass-dev list
  * Settings:

Old tracsvn (OSGeo server) (Dec 2007 * Mai 2019)
```
/var/www/trac/env/grass/conf/trac.ini
  downloadable_paths = /grass-addons/grass7/*/*,/sandbox/*/*
  path = /var/www/grass/htdocs
  link = https://grass.osgeo.org/
  src = site/grasslogo_vector_small.png
  smtp_always_cc = grass-dev@lists.osgeo.org
  smtp_replyto = grass-dev@lists.osgeo.org
  url = https://grass.osgeo.org
  .dir = /var/www/svn/repos/grass
  base_url = https://trac.osgeo.org/grass/
  database = postgres://postgres@/trac_grass
```

Very old bugtracker (Jan 2007 * Dec 2008):
  * http://wald.intevation.org/tracker/?group_id=21
  * gforce, sponsored by Intevation GmbH, Germany
  * spamassasin spamfilter locally, bogofilter at grass-dev list
  * needs 'noreply*wald.intevation.org' to be enabled as alias in Mailman

Very very old bugtracker (Dec 2000 * Dec 2006):
  * https://intevation.de/rt/webrt?q_queue=grass
  * webRT, sponsored by Intevation GmbH, Germany
  * spamassasin spamfilter locally, bogofilter at grass-dev list
  * reports are directly sent to GRASS Developers mailing list for notification
  * TODO: migrate to trac


## GRASS Addons

Maintainer: Martin Landa and Markus Neteler

Details:

- Windows-addons: grass-addons/tools/addons/README.txt
- Addon manual pages cronjob: https://github.com/OSGeo/grass-addons/tree/master/tools/cronjobs_osgeo_lxd
- Rendered manuals: https://grass.osgeo.org/grass7/manuals/addons/

The redirect to the latest grass7x directory is defined on grass.osgeo.org:
  /etc/apache2/includes/grass.osgeo.org.inc

Procedure building of binaries (Windows):
* Addons module are compiled on build server, currently at the CTU in Prague) and publishing their manual pages on publishing server, i.e. grass.osgeo.org.
* A new compilation is triggered everytime when a commit is done in the Addons-SVN.
* Logs:
    * Linux log files:   https://grass.osgeo.org/addons/grass7/logs (compiled on `grasslxd` on `osgeo7`)
    * Windows log files: http://wingrass.fsv.cvut.cz/grass78/x86_64/addons/latest/logs/

Procedure of granting write access to Addons repo:
* Request procedure: https://trac.osgeo.org/grass/wiki/HowToContribute#WriteaccesstotheGRASS-Addons-SVNrepository
* Adding OSGeo-ID:   https://www.osgeo.org/cgi-bin/auth/ldap_group.py?group=grass_addons
* Adding contributor: https://trac.osgeo.org/grass/browser/grass-addons/contributors.csv (via SVN commit)
* Confirm request in grass-psc and give instructions concerning code style etc (see archive for examples)

XML file for g.extension: https://grass.osgeo.org/addons/grass7/modules.xml
* generated in grass-addons/tools/addons/grass-addons-publish.sh

## GRASS Travis CI

Maintainer: Martin Landa

* https://travis-ci.org/GRASS-GIS
* https://github.com/OSGeo/grass
* OLD: https://github.com/GRASS-GIS/grass-ci
* https://github.com/OSGeo/grass-addons/tree/master/tools/grass-ci/

Travis CI control files:
 trunk/.travis/
   linux.before_install.sh
   linux.install.sh
   linux.script.sh

Maintenance script:
* https://github.com/OSGeo/grass-addons/tree/master/tools/grass-ci/grass-ci.sh

The github update is run as a cronjob on server "geo102" (CTU, CZ).


## GRASS CI: GitHub Actions

Started Apr. 2020

Maintainer: Vaclav Petras

* https://github.com/OSGeo/grass/actions
* Details: https://github.com/OSGeo/grass/pull/525

* CI workflow with:
    * A build job which is not parallelized and is meant for clear & relatively fast check of compilation and building in general. (Duplicating what is running on Travis)
    * A test job which of course needs to build, but the main focus is to run tests, so the compilation is parallelized (depending on nproc) and thus potentially less readable. This runs the whole test suite. (You need to run it locally to see the actual error, but you can see which tests are failing.)
* Static code analysis/Code quality check using Flake8 with separate tests for lib/python, gui/wxpython, scripts and temporal directories.
    * lib/python uses configuration which ignores code in testsuite directories and ignores a lot of errors.
    * The other directories use the default settings and the failure is ignored. (Neither is an ideal solution, but we can see and change it based on whatever is more advantageous for getting it fixed.)

Helper files placed to .github/workflows


## GRASS Coverity Scan

Maintainer: Markus Neteler

* https://scan.coverity.com/projects/grass?tab=overview


## Transifex translation management

i18N gettext messages:

* Dashboard: https://www.transifex.com/grass-gis/
* Auto-update URL to fetch files:
    * https://www.transifex.com/grass-gis/grass7/content/
        * Menu: Resources
            * Use: "Auto update resources" button

## OLD: GRASS Quality Control

Maintainer: Prof. Giulio Antoniol

* offline.
* http://web.soccerlab.polymtl.ca/grass-evolution/grass-browsers/grass-index-en.html
   was implemented and sponsored by Ecole Polytechnique de Montreal, Canada
* Realtime analysis has been sent to: http://lists.osgeo.org/mailman/listinfo/grass-qa

Further notification/functionality test systems:
    * posts into #grass IRC channel
    * posts into #osgeo-commits IRC channel

## Previous hosting sponsors

Since 2010 the GRASS GIS project is hosted at the Oregon State University Open Source Lab (OSUOSL), Oregon, USA

Numerous institutions have sponsored the GRASS Project with Hardware/Bandwidth (list of master site hosting):
* 1997-1999: Institut fuer Landschaftspflege und Naturschutz (ILN), Universitaet Hannover, Germany
* 1999-2001: Institut fuer Physische Geographie und Landschaftsoekologie, Universitaet Hannover, Germany
* 2001-2008: ITC-irst, Trento, Italy
* 2009-2010: Telascience.org at San Diego Supercomputer Center, California, USA
* 2010-today: Oregon State University | Open Source Lab, USA
