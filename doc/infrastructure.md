# How the GRASS GIS Webserver and related infrastructure works

Author: Markus Neteler
Last update: Sep 2024

## GRASS GIS Source code repository

Maintainer: Markus Neteler, Martin Landa, OSGeo-SAC, <https://wiki.osgeo.org/wiki/SAC>

Important update April 2019: The source code is now managed on GitHub (rather
than in SVN).

The GitHub repositories are:

- GRASS GIS core (7+): <https://github.com/OSGeo/grass>
- GRASS GIS legacy (3.x-6.x): <https://github.com/OSGeo/grass-legacy>
- GRASS GIS Add-ons: <https://github.com/OSGeo/grass-addons>
- GRASS GIS promotional material: <https://github.com/OSGeo/grass-promo>
- GRASS GIS Website (Hugo site): <https://github.com/OSGeo/grass-website>
- Github mirror at OSGeo (updated daily): <https://git.osgeo.org/gitea/grass_gis/grass>

Git usage:

- [CONTRIBUTING.md file](../CONTRIBUTING.md)
- [Guide to contributing on GitHub](development/github_guide.md)

Issues:

- <https://github.com/OSGeo/grass/issues>
- trac instance: <https://trac.osgeo.org/grass> (old bugs only)

Statistics:

- <https://github.com/OSGeo/grass/pulse>

## GRASS Web server

Maintainer: M. Neteler

- <https://grass.osgeo.org>

  - osgeo8-grass: LXD container on osgeo8 (<https://wiki.osgeo.org/wiki/SAC_Service_Status#osgeo_8>)
    - OS: Debian GNU/Linux 11 bullseye
    - Apache Server with Hugo (<https://github.com/OSGeo/grass-website>)
  - for migration details (7/2020), see <https://github.com/OSGeo/grass-website/issues/180>
  - ssh login: via jumphost hop.osgeo8.osgeo.org
  - deployment via cronjob: <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/>

- <https://old.grass.osgeo.org> (CMSMS, replaced in 2020 by above Hugo based solution)

  - a static copy of the old site is found at
    `neteler@grasslxd:old-grass-website/old.grass.osgeo.org/` (13 GB)
  - otherwise: <https://web.archive.org/web/20191220035849/https://grass.osgeo.org/>

- Backups:

  - osgeo8-grass + wiki: container on osgeo8 is backup'ed,
    see <https://wiki.osgeo.org/wiki/SAC:Backups>

- Mirrors:

  - rsync, see <https://grass.osgeo.org/contribute/> --> Mirror
  - mirror list, see <https://grass.osgeo.org/about/mirrors/>

- RSS feed: offered by Hugo at <https://grass.osgeo.org/index.xml>, used at <https://planet.osgeo.org>

- Weekly software snapshots (generated Saturday morning Portland (OR), US time):

  - Source code tarball of git (GitHub) <https://github.com/OSGeo/grass>
  - Linux binary snapshot is compiled on osgeo8-grass
    - GRASS is compiled with GDAL, PROJ, SQLite, MySQL, PostgreSQL, FFTW, C++ support,
      see <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/>
    - binary tar.gz and manuals are moved into Web space

- GRASS user manual HTML:

  - generated during compilation of weekly Linux binary snapshot on osgeo8-grass

- GRASS addons manual HTML:

  - generated during compilation of weekly Linux binary snapshot on osgeo8-grass

- GRASS programmer's manual (<https://grass.osgeo.org/programming8/>)

  - HTML: cronjob run Wednesday morning Portland (OR), US time
  - HTML: cronjob run Saturday morning Portland (OR), US time
  - disabled: PDF: cronjob run Saturday morning Portland (OR), US time

- Mailman mailing lists + automated greylisting
  (at <https://lists.osgeo.org/mailman/listinfo>, since 11/2007)

  - Mailman is doing the job, only registered users can post
  - messages from unsubscribed people is auto-discarded without notification
  - the open "weblist" operates instead like this:
    - mails goes to grass-web at lists osgeo.org -> greylisting -> Mailman
    - for greylisting, see <https://postgrey.schweikert.ch/>
    - Moderation of queue: <https://lists.osgeo.org/mailman/admindb/grass-web>

- Backup of mailing lists (mbox files)

  - nightly backup at OSGeo.org, bacula

- Web statistics
  - Matomo: <https://2022.foss4g.org/matomo/> (not publicly accessible;
    access: Markus Neteler)
  - Selected stats: <https://wiki.osgeo.org/wiki/Project_Stats> and <https://wiki.osgeo.org/wiki/VisibilityStats#Mailing_list_subscribers>

Summary: The system should run almost autonomously.

## WinGRASS maintenance scripts

See <https://github.com/landam/wingrass-maintenance-scripts>

## GRASS Mailing lists

Maintainer: Markus Neteler

Available lists:

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

- grass-announce:
  - moderated by M. Neteler
  - monthly password reminder is disabled to avoid leakage into publicly
    archived lists
- grass-commit is receiving posts from the GRASS Github. Not open for
  other postings, they will be trashed automatically
- grass-web is an open list (posting without subscription possible with
  moderation), moderated by M. Neteler to avoid spam

## GRASS Wiki

Maintainer: Martin Landa, Markus Neteler

- <https://grasswiki.osgeo.org>
- Mediawiki software
- requires registration at
  [OSGeo-LDAP](https://www.osgeo.org/community/getting-started-osgeo/osgeo_userid/)
  (to keep spammers out)

Macros for manual pages (src, cmd, API, ...):

- <https://grasswiki.osgeo.org/wiki/Category:Templates>

## GRASS IRC

Channel: irc://irc.libera.chat/grass
Web based client: See <https://grasswiki.osgeo.org/wiki/IRC>

Libera IRC:

- Founder: jmckenna, markusN

Former freenode IRC:

- channel owner: Alessandro Frigeri ("geoalf")
- quasi guru level: Markus Neteler ("markusN")
- original operators:
  - Jachym ("jachym")
  - Luca ("doktoreas")
  - Soeren ("huhabla")
  - Brad ("bdouglas")

## GRASS Bugtracker

Current bugtracker (Jan 2020 - today):

- <https://github.com/OSGeo/grass/issues>

Old bugtrackers: see <https://grasswiki.osgeo.org/wiki/Bug_tracking>

## GRASS GIS Addons

Maintainer: Martin Landa, Tomas Zigo, and Markus Neteler

Details:

- Windows-addons: `grass-addons/utils/addons/README.txt`
- Addon manual pages cronjob: <https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd>
- Rendered manuals: <https://grass.osgeo.org/grass8/manuals/addons/>

The redirect to the latest `grassX` directory is defined on grass.osgeo.org:
`/etc/apache2/includes/grass.osgeo.org.inc`

Procedure building of binaries (Windows):

- Addons module are compiled on winGRASS build server, at the CTU in Prague) and
  publishing their manual pages on publishing server, i.e. grass.osgeo.org.
- A new compilation is triggered every time a commit is done in the Addons repo.
- Logs:
  - Linux log files: <https://grass.osgeo.org/addons/grass8/logs> (compiled on
    `grasslxd` on `osgeo8`)
  - Windows log files: <https://wingrass.fsv.cvut.cz/grass8X/addons/grass-XXXdev/logs/>

Procedure of granting write access to Addons repo:

- Request procedure: <https://github.com/OSGeo/grass/blob/main/doc/development/submitting/submitting.md>
- Adding OSGeo-ID: <https://www.osgeo.org/community/getting-started-osgeo/osgeo_userid/>
- Adding contributor: <https://trac.osgeo.org/grass/browser/grass-addons/contributors.csv>
  (via git commit)
- Confirm request in grass-psc and give instructions concerning code style etc
  (see archive for examples)

XML file for g.extension: <https://grass.osgeo.org/addons/grass8/modules.xml>

- generated in `grass-addons/utils/addons/grass-addons-publish.sh`

## GRASS CI: GitHub Actions

Started in Apr. 2020

Maintainer: Vaclav Petras

- <https://github.com/OSGeo/grass/actions>
- Details: <https://github.com/OSGeo/grass/pull/525>
- CI workflow with:
  - A build job which is not parallelized and is meant for clear & relatively fast
    check of compilation and building in general.
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

## GRASS docker images

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

## GRASS Coverity Scan

Maintainer: Markus Neteler

Coverity Scan is a service to find security issues. At time the service
is used only occasionally.

- <https://scan.coverity.com/projects/grass?tab=overview>

## User message translation management (i18N)

Messages are extracted with `gettext` message macros.

Translations may be done using the OSGeo Weblate platform:

- Weblate: <https://weblate.osgeo.org/>
  - GRASS GIS Weblate server: <https://weblate.osgeo.org/projects/grass-gis/>

Anyone with OSGeo-LDAP access can work on the translations.

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

## Previous hosting sponsors

Since 2010 the GRASS GIS project is hosted at the Oregon State University Open
Source Lab (OSUOSL), Oregon, USA

Numerous institutions have sponsored the GRASS Project with Hardware/Bandwidth
(list of master site hosting):

- 1997-1999: Institut fuer Landschaftspflege und Naturschutz (ILN), Universitaet
  Hannover, Germany
- 1999-2001: Institut fuer Physische Geographie und Landschaftsoekologie,
  Universitaet Hannover, Germany
- 2001-2008: ITC-irst, Trento, Italy
- 2009-2010: Telascience.org at San Diego Supercomputer Center, California, USA
- 2010-today: Oregon State University | Open Source Lab, USA
