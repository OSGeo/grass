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
utils/fix_typos.sh
```

### i18N: sync from Transifex

See <https://www.transifex.com/grass-gis/grass7/dashboard/>

Exception Latvian as Latvian is directly edited in git and then sync'ed from
master .po files

```bash
cd locale
sh ~/software/grass-addons/utils/transifex_merge.sh
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
git checkout -b config_sub_update_r80
git add config.guess config.sub configure
git commit -m"config.guess + config.sub: updated from http://git.savannah.gnu.org/cgit/config.git/plain/"
# test by running ./configure

git push origin config_sub_update_r80
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
rm -f python/grass/ctypes/ctypesgencore/parser/lextab.py
rm -f gui/wxpython/menustrings.py gui/wxpython/build_ext.pyc \
  gui/wxpython/xml/menudata.xml gui/wxpython/xml/module_tree_menudata.xml
chmod -R a+r *
```

Double check:

```bash
git status
```

### Create release branch (only if not yet existing)

.. see section below at end of file.

### Update VERSION file to release version number

Directly edit VERSION file in GH interface:

<https://github.com/OSGeo/grass/blob/releasebranch_8_0/include/VERSION>

Example:

```bash
8
0
0RC1
2021
```

### Create release tag

(see <https://help.github.com/en/articles/creating-releases>)

Preparation:

### Changelog and tagging etc preparations

```bash
# update from GH
#  assumptions:
#  - own fork as "origin"
#  - remote repo as "upstream"
git fetch --all --prune && git checkout releasebranch_8_0 && \
 git merge upstream/releasebranch_8_0 && git push origin releasebranch_8_0

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

### Tag release (on GitHub)

```bash
echo "$VERSION"
```

To be done in GH interface:

<https://github.com/OSGeo/grass/releases/new>

Tag version | target (examples):
  8.0.0RC1  | releasebranch_8_0

Add release desciption (re-use existing texts as possible, from
<https://github.com/OSGeo/grass/releases>)

If RC, then check
[x] This is a pre-release

### Packaging of source code tarball

```bash
# fetch tarball from GitHub
wget https://github.com/OSGeo/grass/archive/${VERSION}.tar.gz -O grass-${VERSION}.tar.gz
md5sum grass-${VERSION}.tar.gz > grass-${VERSION}.md5sum
```

### Changelog from GitHub for GH release notes

Using GH API here, see also
- https://cli.github.com/manual/gh_api
- https://docs.github.com/en/rest/reference/repos#generate-release-notes-content-for-a-release

```bash
gh api repos/OSGeo/grass/releases/generate-notes -f tag_name="8.0.0" -f previous_tag_name=7.8.6 -f target_commitish=releasebranch_8_0 -q .body
```

Importantly, these notes need to be manually sorted into the various categories.

### Changelog file for upload

```bash
python3 utils/gitlog2changelog.py
mv ChangeLog ChangeLog_$VERSION
head ChangeLog_$VERSION
gzip ChangeLog_$VERSION
```

### Reset include/VERSION file to git version

Directly edit VERSION file in GH interface:

<https://github.com/OSGeo/grass/blob/releasebranch_8_0/include/VERSION>

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
git fetch --all --prune && git checkout releasebranch_8_0 && \
 git merge upstream/releasebranch_8_0 && git push origin releasebranch_8_0
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
  INSTALL REQUIREMENTS.html SUBMITTING $USER@$SERVER1:$SERVER1DIR

scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html SUBMITTING $USER@$SERVER2:$SERVER2DIR

# Only at full release!
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

# update addons - major/minor release only
vim grass-addons/utils/addons/grass-addons-publish.sh
vim grass-addons/utils/addons/grass-addons-build.sh
vim grass-addons/utils/addons/grass-addons.sh
```

# update addon builder
- https://github.com/landam/wingrass-maintenance-scripts/blob/master/grass_addons.sh (add new release related line)

### Close milestone

- Close related milestone: https://github.com/OSGeo/grass/milestones

Release is done.


### Advertise the new release

#### Write trac Wiki release page

To easily generate the entries for the trac Wiki release page, use the `git log` approach:
- extract entries from oneline git log and prepare for trac Wiki copy-paste:

```
# get date of previous release from https://github.com/OSGeo/grass/releases
# verify
git log --oneline --after="2021-10-10" | tac

# prepare for trac Wiki release page (incl. PR trac macro)
git log --oneline --after="2021-10-10" | cut -d' ' -f2- | sed 's+^+ * G80:+g' | sed 's+(#+(PR:+g' | sort -u
```

- store changelog entries in trac, by section:
    - <https://trac.osgeo.org/grass/wiki/Release/8.0.x-News>
    - <https://trac.osgeo.org/grass/wiki/Grass8/NewFeatures80>  <- add content of major changes only

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

- update cronjob '[cron_grass_HEAD_src_snapshot.sh](https://github.com/OSGeo/grass-addons/tree/master/utils/cronjobs_osgeo_lxd)' on grass.osgeo.org to next
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
     set MINOR=0
     set PATCH=0RC1
```

- Update addons (grass_addons.sh) rules, eg.

```
     compile $GIT_PATH/grass8 $GISBASE_PATH/grass800RC1  $ADDON_PATH/grass800RC1/addons
```

- Modify grass_copy_wwwroot.sh accordingly, eg.

```
     copy_addon 800RC1 8.0.0RC1
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
- Geowanking: <geowanking@geowanking.org>
- OSGeo.org: <news_item@osgeo.org>, <info@osgeo.org>
- Geo Connexion: <editor-geo@geoconnexion.com>

Via Web:

- <http://linuxtoday.com/contribute.php3>
- <https://joinup.ec.europa.eu/software/grassgis/home> (submit news, MN)
- <http://www.directionsmag.com/pressreleases/> (News -> Submit Press Release)
- <http://directory.fsf.org/wiki/GRASS_%28Geographic_Resources_Analysis_Support_System%29>
- <https://www.linux-apps.com/p/1128004/edit/> (MN)
- <https://www.heise.de/download/product/grass-gis-7105> (update, MN)
- See also: <https://grass.osgeo.org/wiki/Contact_Databases>
- ... anywhere else? Please add here.
