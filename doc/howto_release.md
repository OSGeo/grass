# How to release GRASS GIS binaries and source code

*Note: Some steps in this text are to be done by the development coordinator
(currently Markus Neteler and Martin Landa) due to needed logins.*

## HOWTO create a release

### Create release branch (only if not yet existing)

.. see section below at end of file.

### Update VERSION file to release version number

Directly edit VERSION file in GH interface:

<https://github.com/OSGeo/grass/blob/releasebranch_8_2/include/VERSION>

Example:

```bash
8
2
0RC1
2022
```

Commit with version message, e.g. "GRASS GIS 8.2.0RC1".

### Create release tag

(For background, see <https://help.github.com/en/articles/creating-releases>)

Preparation:

#### Changelog and tagging etc preparations

```bash
# update from GH
#  assumptions:
#  - own fork as "origin"
#  - remote repo as "upstream"
git fetch --all --prune && git checkout releasebranch_8_2 && \
 git merge upstream/releasebranch_8_2 && git push origin releasebranch_8_2

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
- fill in "Release Title" (e.g., GRASS GIS 8.2.0RC1)
- fill in "Create tag" field: 8.2.0RC1

Tag version | target (examples):
  8.2.0RC1  | releasebranch_8_2

- click on "Create new tag: ... on publish"

Add release desciption (re-use existing texts as possible, from
<https://github.com/OSGeo/grass/releases>)

If RC, then check
[x] This is a pre-release

### Changelog from GitHub for GH release notes

Generate a draft of release notes using a script. The script uses configuration files
which are in the _utils_ directory and the script needs to run there,
so change the current directory:

```bash
cd utils
```

For major and minor releases, GitHub API gives good results
because it contains contributor handles and can identify new contributors,
so use with the _api_ backend, e.g.:

```bash
python ./generate_release_notes.py api releasebranch_8_2 8.0.0 $VERSION
```

For micro releases, GitHub API does not give good results because it uses PRs
while the backports are usually direct commits without PRs.
The _git log_ command operates on commits, so use use the _log_ backend:

```bash
python ./generate_release_notes.py log releasebranch_8_2 8.2.0 $VERSION
```

The script sorts them into categories defined in _utils/release.yml_.
However, these notes need to be manually edited to collapse related items into one.
Additionally, a _Highlights_ section needs to be added with manually identified new
major features for major and minor releases and for all releases, as _Major_ section
may need to be added showing critical fixes or breaking changes.

### Changelog file for upload

```bash
python3 utils/gitlog2changelog.py
mv ChangeLog ChangeLog_$VERSION
head ChangeLog_$VERSION
gzip ChangeLog_$VERSION
```

### Reset include/VERSION file to git version

Directly edit VERSION file in GH interface:

<https://github.com/OSGeo/grass/blob/releasebranch_8_2/include/VERSION>

Example:

```bash
8
0
1dev
2021
```

Commit as "back to dev"

Reset local copy to GH:

```bash
# update from GH
#  assumptions:
#  - own fork as "origin"
#  - remote repo as "upstream"
git fetch --all --prune && git checkout releasebranch_8_2 && \
 git merge upstream/releasebranch_8_2 && git push origin releasebranch_8_2
```

### Getting the source code tarball for upload on OSGeo server

```bash
# fetch tarball from GitHub
wget https://github.com/OSGeo/grass/archive/${VERSION}.tar.gz -O grass-${VERSION}.tar.gz
md5sum grass-${VERSION}.tar.gz > grass-${VERSION}.md5sum
```

### Upload source code tarball to OSGeo servers

Note: grasslxd only reachable via jumphost - https://wiki.osgeo.org/wiki/SAC_Service_Status#GRASS_GIS_server

```bash
# Store the source tarball (twice) in (use scp -p FILES grass:):
USER=neteler
SERVER1=grasslxd
SERVER1DIR=/var/www/code_and_data/grass$MAJOR$MINOR/source/
SERVER2=download.osgeo.org
SERVER2DIR=/osgeo/download/grass/grass$MAJOR$MINOR/source/
echo $SERVER1:$SERVER1DIR
echo $SERVER2:$SERVER2DIR

# upload along with associated files:
scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html CONTRIBUTING.md $USER@$SERVER1:$SERVER1DIR

scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html CONTRIBUTING.md $USER@$SERVER2:$SERVER2DIR

# Only at full release (i.e., not for RCs)!
# generate link to "latest" source code
ssh $USER@$SERVER1 "cd $SERVER1DIR ; rm -f grass-$MAJOR.$MINOR-latest.tar.gz"
ssh $USER@$SERVER1 "cd $SERVER1DIR ; ln -s grass-$VERSION.tar.gz grass-$MAJOR.$MINOR-latest.tar.gz"
ssh $USER@$SERVER1 "cd $SERVER1DIR ; rm -f grass-$MAJOR.$MINOR-latest.md5sum"
ssh $USER@$SERVER1 "cd $SERVER1DIR ; ln -s grass-$VERSION.tar.md5sum grass-$MAJOR.$MINOR-latest.md5sum"

# verify
echo "https://grass.osgeo.org/grass$MAJOR$MINOR/source/"

# update winGRASS related files: Update the winGRASS version
# https://github.com/landam/wingrass-maintenance-scripts/
vim wingrass-maintenance-scripts/grass_packager_release.bat
vim wingrass-maintenance-scripts/grass_addons.sh
vim wingrass-maintenance-scripts/grass_copy_wwwroot.sh
vim wingrass-maintenance-scripts/cronjob.sh       # major/minor release only
```

# update addon builder
- https://github.com/landam/wingrass-maintenance-scripts/blob/master/grass_addons.sh (add new release related line)

### Close milestone

- Close related milestone: https://github.com/OSGeo/grass/milestones

Release is done.


### Advertise the new release

#### Trac Wiki release page

Add entry in https://trac.osgeo.org/grass/wiki/Release

#### Update Hugo web site to show new version (not for RCs!)

Write announcement and publish it:
- News section, https://github.com/OSGeo/grass-website/tree/master/content/news

Software pages:
- Linux: https://github.com/OSGeo/grass-website/blob/master/content/download/linux.en.md
- Windows: https://github.com/OSGeo/grass-website/blob/master/content/download/windows.en.md
- Mac: https://github.com/OSGeo/grass-website/blob/master/content/download/mac.en.md
- Releases: https://github.com/OSGeo/grass-website/blob/master/content/about/history/releases.md
- Wiki: https://grasswiki.osgeo.org/wiki/GRASS-Wiki


#### Only in case of new major release

- update cronjob '[cron_grass8_main_src_snapshot.sh](https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/)' on grass.osgeo.org to next
  but one release tag for the differences
- wiki updates, only when new major release:
    - {{cmd|xxxx}} macro: <https://grasswiki.osgeo.org/wiki/Template:Cmd>
    - update last version on main page
- Add trac Wiki Macro definitions for manual pages G8X:modulename
    - Edit: <https://trac.osgeo.org/grass/wiki/InterMapTxt>

#### WinGRASS notes

- Update grass_packager_release.bat, eg.

```
     set MAJOR=8
     set MINOR=2
     set PATCH=0RC1
```

- Update addons (grass_addons.sh) rules, eg.

```
     compile $GIT_PATH/grass8 $GISBASE_PATH/grass820RC1  $ADDON_PATH/grass820RC1/addons
```

- Modify grass_copy_wwwroot.sh accordingly, eg.

```
     copy_addon 820RC1 8.2.0RC1
```

#### Ubuntu Launchpad notes

- Create milestone and release: <https://launchpad.net/grass/+series>
- Upload tarball for created release

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
