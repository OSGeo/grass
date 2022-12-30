# RFC 6: Migration from SVN to GitHub

Authors of the first draft: Markus Neteler, Martin Landa

Status: Motion passed Apr 22, 2019

## Introduction

GRASS GIS is an open source geoinformation system which is developed by a globally distributed team of developers. Besides the source code developers also message translators, people who write documentation, those who report bugs and wishes and more are involved.

The centralized source code management system Subversion (SVN) has served the GRASS GIS project very well since 2007. The project has established routines and infrastructure (code repository, ticketing system, developer wiki) connected to SVN. However, with an increasing number of Open Source developers using git (and here especially the success of GitHub), time has come to migrate the source code base from SVN kindly hosted by OSGeo to GitHub.com, a widely adopted development platform.

## Background information of git migration

* For background and aims, see
  * <https://trac.osgeo.org/grass/wiki/GitMigration>
* Results of a survey performed in early 2019:
  * <https://docs.google.com/forms/d/1BoTFyZRNebqVX98A3rh5GpUS2gKFfmuim78gbradDjc/viewanalytics>
* Technical discussions
  * svn -> git test migration ongoing, see [#3722](https://trac.osgeo.org/grass/ticket/3722)
  * trac -> github test issue migration ongoing, see [#3789](https://trac.osgeo.org/grass/ticket/3789)

## New GitHub repositories

Since migration is a huge effort, massive work on converting the existing source code (organized in branches) and the related trac tickets has been done. The main scope of weeks of efforts was to preserve as much information as possible by converting trac/svn references to full URLs pointing to the old system kept available in read-only mode.

The following new GitHub repositories have been [created](https://github.com/grass-svn2git). Note that the "cut-off" date of the main **grass** repository does not correspond to the first upload to CSV which was then migrated to SVN. The repositories **grass** and **grass-legacy** overlap in time since they contain different branches:

* repository **grass**
  * Source code from 2008 (as the starting commit r31142 was selected, i.e. "Welcome to GRASS 7.0.svn") to present day (SVN-trunk -> git-master)
  * i.e., all 7.x and later release branches + master
* repository **grass-legacy**
  * Source code from 1987 (pre-public internet times; manually reconstructed) - 2018 (r72361 - last commit to releasebranch_6_4)
  * i.e., a separate repository for older GRASS GIS releases (3.2, 4.x, 5.x, 6.x)
* repository **grass-addons**
  * repository for addons
  * code re-organized from directory-like layout (grass6, grass7) into branches-like layout (master == grass7, grass6, ...)
* repository **grass-promo**
  * repository for promotional material

The final destination of these repositories will be under <https://github.com/OSGeo/>

## Authorship and SVN user name mapping to GitHub

Given GRASS GIS’ history of 35+ years we had to invest major effort in identifying and mapping user names throughout the decades (CVS was used from 1999 to 2007; SVN has been used since 2007, see [history](https://grasswiki.osgeo.org/wiki/Bug_tracking)).

The following circumstances could be identified:

* user present in CVS but not in SVN
* user present in SVN but not in CVS
* user present in both with identical name
* user present in both with different name as some were changed in the CVS to SVN migration in 2007, leading to colliding user names
* some users already having a GitHub account (with mostly different name again)
  * see here for the [author mapping table](https://trac.osgeo.org/grass/browser/grass-addons/tools/svn2git/)

**Important**: nothing is lost as contributions can be [claimed](https://help.github.com/en/articles/why-are-my-commits-linked-to-the-wrong-user) in GitHub.

## Activating the GitHub issue tracker

As the cut-off date for the trac migration we selected 2007-12-09 (r25479) as it was the first SVN commit (after the years in CVS).

The tickets migrated from trac to GitHub contain updated links in the ticket texts:

* links to other tickets (closed now pointing to full trac URL, open pointing to a new github issues). Note that there were many styles of referring in the commit log message which had to be parsed accordingly.
* links to trac wiki (now pointing to full trac URL)
* links source code in SVN (now pointing to full trac URL)
* images and attachments (now pointing to full trac URL)

Labels are preserved by transferring:

* "operating system" trac label into the GitHub issue text itself (following the new issue reporting template)
* converting milestones/tickets/comments/labels
* converting trac usernames to known GitHub usernames (those missing at time can [claim](https://help.github.com/en/articles/why-are-my-commits-linked-to-the-wrong-user) commits)
* setting assignees if possible; otherwise set new "grass-svn2git" an assignee

**New labels in the GitHub issue tracker:**

The trac component of the bug reports have been cleaned up following other OSGeo projects like GDAL and QGIS, leading to the following categories:

* **Issue category**:
  * bug
  * enhancement
* **Priority**:
  * blocker
  * critical
  * feedback needed
* **Issue solution** (other than fixing and closing it normally):
  * duplicate
  * invalid
  * wontfix
  * worksforme
* **Components**:
  * docs
  * GUI
  * libs
  * modules
  * packaging
  * python
  * translations
  * unittests
  * Windows specific

Note that "normal" bugs reported will not carry a label in order to not overload the visual impact and readability.

## Rules and best practices for using Git

Before the new Git repository is open for writing, we need to have rules and best practices for dealing with the following:

* Policy for automatic merge commits due to un-synchronous nature of Git. Do we want to avoid those by `git pull --rebase`?
* How to do backports?
* A branch for every feature or bug fix in the main repo or is this done in the fork?
* _(add more)_

## Turning SVN/trac into readonly mode

As soon as the above listed repositories are stable and functional, SVN/trac (<https://trac.osgeo.org/grass/>) at OSGeo will be set into readonly mode. They will serve as a reference for existing links and also for the aforementioned converted commit messages and issues in the issue tracker.

## Open issues

* Will be also Trac wiki migrated into GitHub?
  * This can be decided at a later stage. Managed in [#3789](https://trac.osgeo.org/grass/ticket/3789)

## Mirror or Exit strategy

GitHub is a closed platform. In case it would be shutdown, closed or GitHub would start asking unreasonable fees, a backup strategy is needed.
The proposed solution is

* GitLab has an import module from GitHub.
  * implement a continuous mirror on GitLab.com including the issues and wiki.
    * OSGeo-gitea code mirror: <https://git.osgeo.org/gitea/grass_gis/grass>
  * See <https://docs.gitlab.com/ee/workflow/repository_mirroring.html>
    * The mirroring is only for the repository, not the issues or wiki.



