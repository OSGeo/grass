# How to release GRASS GIS binaries and source code

*Note: Some steps in this text are to be done by the development coordinator
(currently Markus Neteler and Martin Landa) due to needed logins.*

## HOWTO create a release

### Preparations

Check examples if still compiling

```bash
( cd doc/raster/r.example/ ; make clean ; make )
( cd doc/vector/v.example/ ; make clean ; make )
```

### Fix typos in source code with

```bash
tools/fix_typos.sh
```

### i18N: sync from Transifex

See <https://www.transifex.com/grass-gis/grass7/dashboard/>

Exception Latvian as Latvian is directly edited in git and then sync'ed from
master .po files

```bash
cd locale
sh ~/software/grass-addons/tools/transifex_merge.sh
make
make verify
# ... then fix .po files as needed.
#
# requires https://trac.osgeo.org/grass/ticket/3539
## after that push fixes to transifex:
#cd locale/transifex/
#tx --debug push -t
```

### Update of configure base files

*Only allowed in RC cycle, not for final release!*

Check that autoconf scripts are up-to-date:

```bash
rm -f config.guess config.sub
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.guess
wget http://git.savannah.gnu.org/cgit/config.git/plain/config.sub
git diff config.guess config.sub
autoconf2.69
```

Now check if configure still works.

If yes, submit to git:

```bash
git checkout -b config_sub_update_r78
git add config.guess config.sub configure
git commit -m"config.guess + config.sub: updated from http://git.savannah.gnu.org/cgit/config.git/plain/" config.guess config.sub configure
# test by running ./configure

git push origin config_sub_update_r78
# open PR and merge
```

### Cleanup leftover rubbish files

```bash
rm -f locale/templates/*.pot
rm -f locale/po/messages.mo
rm -f demolocation/PERMANENT/.bash*
find . -name '*~'     | xargs -r rm
find . -name '*.bak'  | xargs -r rm
find . -name '*.swp'  | xargs -r rm
find . -name '.#*'    | xargs -r rm
find . -name '*.orig' | xargs -r rm
find . -name '*.rej'  | xargs -r rm
find . -name '*.o'    | xargs -r rm
find . -name '*.pyc'  | xargs -r rm
find . -name 'OBJ.*'  | xargs -r rm -r
find . -name '__pycache__' | xargs -r rm -r
rm -f lib/python/ctypes/ctypesgencore/parser/lextab.py
rm -f gui/wxpython/menustrings.py gui/wxpython/build_ext.pyc \
  gui/wxpython/xml/menudata.xml gui/wxpython/xml/module_tree_menudata.xml
chmod -R a+r *
```

Double check:

```bash
git status --ignored
```

### Create release branch (only if not yet existing)

.. see section below at end of file.

### Update VERSION file to release version number

Directly edit VERSION file in GH interface:

<https://github.com/OSGeo/grass/blob/releasebranch_7_8/include/VERSION>

Example:

```bash
7
8
7RC1
2022
```

Commit with version message, e.g. "GRASS GIS 7.8.7RC1".

### Create release tag

(For background, see <https://help.github.com/en/articles/creating-releases>)

Preparation:

#### Changelog and tagging etc preparations

```bash
# update from GH
#  assumptions:
#  - own fork as "origin"
#  - remote repo as "upstream"
git fetch --all --prune && git checkout releasebranch_7_8 && \
 git merge upstream/releasebranch_7_8 && git push origin releasebranch_7_8

# create version env var for convenience:
MAJOR=`cat include/VERSION | head -1 | tail -1`
MINOR=`cat include/VERSION | head -2 | tail -1`
RELEASE=`cat include/VERSION | head -3 | tail -1`
VERSION=${MAJOR}.${MINOR}.${RELEASE}
echo $VERSION

# RELEASETAG variable not really needed any more:
TODAY=`date +"%Y%m%d"`
RELEASETAG=release_${TODAY}_grass_${MAJOR}_${MINOR}_${RELEASE}
echo $RELEASETAG
```

#### Tag release (on GitHub)

```bash
echo "$VERSION"
```

To be done in GH interface:

<https://github.com/OSGeo/grass/releases/new>

- select release_branch first, then
- fill in "Release Title" (e.g., GRASS GIS 7.8.7RC1)
- fill in "Create tag" field: 7.8.7RC1

Tag version | target (examples):
  7.8.7RC1  | releasebranch_7_8

- click on "Create new tag: ... on publish"

Add release desciption (re-use existing texts as possible, from
<https://github.com/OSGeo/grass/releases>)

If RC, then check
[x] Set as a pre-release

### Changelog from GitHub for GH release notes

Using GH API here, see also
- https://cli.github.com/manual/gh_api
- https://docs.github.com/en/rest/reference/repos#generate-release-notes-content-for-a-release

```bash
gh api repos/OSGeo/grass/releases/generate-notes -f tag_name="7.8.7" -f previous_tag_name=7.8.6 -f target_commitish=releasebranch_7_8 -q .body
```

If this fails, also a date may be used (that of the last release):

```bash
# GitHub style
git log --pretty=format:"* %s by %an" --after="2022-01-28" | sort

# trac style
git log --oneline --after="2022-01-28" | cut -d' ' -f2- | sed 's+^+* +g' | sed 's+(#+https://github.com/OSGeo/grass/pull/+g' | sed 's+)$++g' | sort -u
```

Importantly, these notes need to be manually sorted into the various categories (modules, wxGUI, library, docker, ...).

### Changelog file for upload

```bash
python3 tools/gitlog2changelog.py
mv ChangeLog ChangeLog_$VERSION
head ChangeLog_$VERSION
gzip ChangeLog_$VERSION
```

### Reset include/VERSION file to git version

Directly edit VERSION file in GH interface:

<https://github.com/OSGeo/grass/blob/releasebranch_7_8/include/VERSION>

Example:

```bash
7
8
7dev
2022
```

Commit as "back to dev"

Reset local copy to GH:

```bash
# update from GH
#  assumptions:
#  - own fork as "origin"
#  - remote repo as "upstream"
git fetch --all --prune && git checkout releasebranch_7_8 && \
 git merge upstream/releasebranch_7_8 && git push origin releasebranch_7_8

### Getting the source code tarball for upload on OSGeo server

```bash
# fetch tarball from GitHub
wget https://github.com/OSGeo/grass/archive/${VERSION}.tar.gz -O grass-${VERSION}.tar.gz
md5sum grass-${VERSION}.tar.gz > grass-${VERSION}.md5sum
```

### Upload source code tarball to OSGeo servers

Note: servers 'osgeo7-grass' and 'osgeo7-download' only reachable via
      jumphost (managed by OSGeo-SAC) - see https://wiki.osgeo.org/wiki/SAC_Service_Status#grass

```bash
# Store the source tarball (twice) in (use scp -p FILES grass:):
USER=neteler
SERVER1=osgeo7-grass
SERVER1DIR=/var/www/code_and_data/grass$MAJOR$MINOR/source/
SERVER2=osgeo7-download
SERVER2DIR=/osgeo/download/grass/grass$MAJOR$MINOR/source/
echo $SERVER1:$SERVER1DIR
echo $SERVER2:$SERVER2DIR

# upload along with associated files:
scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html SUBMITTING CONTRIBUTING.md $USER@$SERVER1:$SERVER1DIR

scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html SUBMITTING CONTRIBUTING.md $USER@$SERVER2:$SERVER2DIR

# Only at full release (i.e., not for RCs)!
# generate link to "latest" source code
ssh $USER@$SERVER1 "cd $SERVER1DIR ; rm -f grass-$MAJOR.$MINOR-latest.tar.gz"
ssh $USER@$SERVER1 "cd $SERVER1DIR ; ln -s grass-$VERSION.tar.gz grass-$MAJOR.$MINOR-latest.tar.gz"
ssh $USER@$SERVER1 "cd $SERVER1DIR ; rm -f grass-$MAJOR.$MINOR-latest.md5sum"
ssh $USER@$SERVER1 "cd $SERVER1DIR ; ln -s grass-$VERSION.tar.md5sum grass-$MAJOR.$MINOR-latest.md5sum"

# verify
echo "https://grass.osgeo.org/grass$MAJOR$MINOR/source/"

# update winGRASS related files: Update the winGRASS version
# https://github.com/landam/wingrass-maintenance-scripts
vim wingrass-maintenance-scripts/grass_packager_release.bat
vim wingrass-maintenance-scripts/grass_addons.sh
vim wingrass-maintenance-scripts/grass_copy_wwwroot.sh
vim wingrass-maintenance-scripts/cronjob.sh       # major/minor release only

# update addons - major/minor release only <<-- outdated?!
vim grass-addons/tools/addons/grass-addons-publish.sh
vim grass-addons/tools/addons/grass-addons-build.sh
vim grass-addons/tools/addons/grass-addons.sh
```

# update addon builder
- https://github.com/landam/wingrass-maintenance-scripts/blob/master/grass_addons.sh (add new release related line)

### Close milestone

- Close related milestone: https://github.com/OSGeo/grass/milestones

Release is done.


### Advertise the new release

#### Write trac Wiki release page (probably to be dropped)

To easily generate the entries for the trac Wiki release page, use the `git log` approach:
- extract entries from oneline git log and prepare for trac Wiki copy-paste:

```
# get date of previous release from https://github.com/OSGeo/grass/releases
# verify
git log --oneline --after="2022-01-28" | tac

# prepare for trac Wiki release page (incl. PR trac macro)
git log --oneline --after="2022-01-28" | cut -d' ' -f2- | sed 's+^+ * G78:+g' | sed 's+(#+(PR:+g' | sort -u
```

- store changelog entries in trac, by section:
    - <https://trac.osgeo.org/grass/wiki/Release/7.8.x-News>
    - <https://trac.osgeo.org/grass/wiki/Grass7/NewFeatures78>  <- add content of major changes only

#### Update CMS web site to show new version (not for RCs!)

Write announcement and publish it:
- News section, https://github.com/OSGeo/grass-website/tree/master/content/news

Software pages:
- Linux: https://github.com/OSGeo/grass-website/blob/master/content/download/linux.en.md
- Windows: https://github.com/OSGeo/grass-website/blob/master/content/download/windows.en.md
- Mac: https://github.com/OSGeo/grass-website/blob/master/content/download/mac.en.md
- Releases: https://github.com/OSGeo/grass-website/blob/master/content/about/history/releases.md
- Wiki: https://grasswiki.osgeo.org/wiki/GRASS-Wiki


#### Only in case of new major release

- update cronjob '[cron_grass78_src_relbr78_snapshot.sh](https://github.com/OSGeo/grass-addons/tree/master/tools/cronjobs_osgeo_lxd/)' on grass.osgeo.org to next
  but one release tag for the differences
- wiki updates, only when new major release:
    - {{cmd|xxxx}} macro: <https://grasswiki.osgeo.org/wiki/Template:Cmd>
    - update last version on main page
- trac updates, only when new major release:
    - Set "complete" flag in <https://trac.osgeo.org/grass/milestone/7.8.x> -->
      Edit Milestone
        - also: Retarget associated open tickets to milestone 7.8.x
    - Batch modify tickets, set to next milestone (update this query
      accordingly: two entries to change):
        - <https://trac.osgeo.org/grass/query?status=assigned&status=new&status=reopened&milestone=7.8.0&milestone=7.8.1&milestone=7.8.2&milestone=7.8.3&milestone=7.8.4&milestone=7.8.5&milestone=7.8.6&milestone=7.8.7&group=status&col=id&col=summary&col=owner&col=type&col=priority&col=component&col=version&order=priority>
    - Set max items to 1000, then select all shown tickets via Status:
      assigned/new/reopened sections
    - Scroll down to "Batch modify": under the "Comment" section, add Field
      "Milestone" and set to next version
    - then use "Change ticket" button, done.
        - Only in case of new release branch being created:
- Add Wiki Macro definitions for manual pages G7X:modulename
    - Edit: <https://trac.osgeo.org/grass/wiki/InterMapTxt>

#### WinGRASS notes

- Update grass_packager_release.bat, eg.

```
     set MAJOR=7
     set MINOR=8
     set PATCH=7RC1
```

- Update addons (grass_addons.sh) rules, eg.

```
     compile $GIT_PATH/grass7 $GISBASE_PATH/grass787RC1   $ADDON_PATH/grass787RC1/addons
```

- Modify grass_copy_wwwroot.sh accordingly, eg.

```
     copy_addon 787RC1 7.8.7RC1
```

#### Launchpad notes

- Create milestone and release: <https://launchpad.net/grass/+series>
- Upload tarball for created release
- Update daily recipe contents: <https://code.launchpad.net/~grass/+recipe/grass-trunk>

#### Packaging notes

- <https://trac.osgeo.org/grass/wiki/BuildHints>
    - <https://trac.osgeo.org/grass/wiki/DebianUbuntuPackaging>
    - <https://trac.osgeo.org/grass/wiki/CompileOnWindows>

#### Marketing - tell others about release

- Notify all packagers (MN has email list)
- If release candidate:
    - <grass-announce@lists.osgeo.org>
    - <grass-dev@lists.osgeo.org>
- If official release:
    - publish related announcement press release at:
- Our GRASS web site: /announces/
    - Note: DON'T use relative links there
- Our main mailing lists:
    - <https://lists.osgeo.org/mailman/listinfo/grass-announce> | <grass-announce@lists.osgeo.org>
    - <https://lists.osgeo.org/mailman/listinfo/grass-dev> | <grass-dev@lists.osgeo.org>
    - <https://lists.osgeo.org/mailman/listinfo/grass-user> | <grass-user@lists.osgeo.org>
- FreeGIS: <freegis-list@intevation.de>
- OSGeo.org: <news_item@osgeo.org>, <info@osgeo.org>

Via Web / Social media:

- See: <https://grass.osgeo.org/wiki/Contact_Databases>
