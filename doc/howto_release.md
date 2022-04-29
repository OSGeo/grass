# How to release GRASS GIS binaries and source code

*Note: Some steps in this text are to be done by the development coordinator
(currently Markus Neteler and Martin Landa) due to needed logins.*

## HOWTO create a release

### Create release branch (only if not yet existing)

.. see section below at end of file.

### Update VERSION file to release version number

Modify the VERSION file use the dedicated script, for RC1, e.g.:

```bash
./utils/update_version.py status
./utils/update_version.py rc 1
```

The script will compute the correct version string and print a message containing it into the terminal (e.g., "version: GRASS GIS 8.2.0RC1").

Commit with a commit message suggested by the script, e.g.:

```bash
git diff
git commit -m "version: GRASS GIS 8.2.0RC1" include/VERSION
git show
git push upstream
```

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
# Get VERSION and TAG as variables.
eval `./update_version.py status --bash`
```

#### Tag release (on GitHub)

Version and tag are the same for all releases:

```bash
echo "$VERSION"
echo "$TAG"
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

Using GH API here, see also
- https://cli.github.com/manual/gh_api
- https://docs.github.com/en/rest/reference/repos#generate-release-notes-content-for-a-release

```bash
gh api repos/OSGeo/grass/releases/generate-notes -f tag_name="8.2.0" -f previous_tag_name=8.2.0 -f target_commitish=releasebranch_8_2 -q .body
```

If this fails or is incomplete, also a date may be used (that of the last release):

```bash
# GitHub style (this works best)
git log --pretty=format:"* %s by %an" --after="2022-01-28" | sort

# trac style (no longer really needed)
git log --oneline --after="2022-01-28" | cut -d' ' -f2- | sed 's+^+* +g' | sed 's+(#+https://github.com/OSGeo/grass/pull/+g' | sed 's+)$++g' | sort -u
```

Importantly, these notes need to be manually sorted into the various categories (modules, wxGUI, library, docker, ...).

### Changelog file for upload

```bash
python3 utils/gitlog2changelog.py
mv ChangeLog ChangeLog_$VERSION
head ChangeLog_$VERSION
gzip ChangeLog_$VERSION
```

### Reset include/VERSION file to git development version

Use a dedicated script to edit the VERSION file, for RC1, e.g.:

Example:

```bash
./utils/update_version.py dev
```

Commit with the suggested commit message and push, e.g.:

```bash
git show
git commit include/VERSION -m "version: Back to 8.2.0dev"
git push upstream
```

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
