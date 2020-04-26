# How to release GRASS GIS binaries and source code

*Note: Some steps in this text are to be done by the development coordinator
(currently Markus Neteler, PSC Chair) due to needed logins.*

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
sh ~/software/grass_addons_git/tools/transifex_merge.sh
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
autoconf-2.13
```

Now check if configure still works.

If yes, submit to git:

```bash
git add config.guess config.sub configure
git checkout -b config_sub_update_r78
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
find . -name '*~'     | xargs rm
find . -name '*.bak'  | xargs rm
find . -name '*.swp'  | xargs rm
find . -name '.#*'    | xargs rm
find . -name '*.orig' | xargs rm
find . -name '*.rej'  | xargs rm
find . -name '*.o'    | xargs rm
find . -name '*.pyc'  | xargs rm
find . -name 'OBJ.*'  | xargs rm -r
find . -name '__pycache__' | xargs rm -r
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

<https://github.com/OSGeo/grass/blob/releasebranch_7_8/include/VERSION>

Example:

```bash
7
8
1RC1
2019
```

~~Update OSGeo4W setup.hint file~~
~~no longer needed~~
~~vim mswindows/osgeo4w/setup_x86.hint.tmpl~~
~~vim mswindows/osgeo4w/setup_x86_64.hint.tmpl~~

### Create release tag

(see <https://help.github.com/en/articles/creating-releases>)

Preparation:

### Changelog and tagging etc

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

### Tag release (on GitHub)

```bash
echo "$VERSION"
```

To be done in GH interface:

<https://github.com/OSGeo/grass/releases/new>

Tag version | target (examples):
  7.8.1RC1  | releasebranch_7_8

Add release desciption (re-use existing texts as possible)

If RC, then check
[x] This is a pre-release

### Packaging of source code tarball

```bash
# fetch tarball from GitHub
wget https://github.com/OSGeo/grass/archive/${VERSION}.tar.gz -O grass-${VERSION}.tar.gz
md5sum grass-${VERSION}.tar.gz > grass-${VERSION}.md5sum
```

Create Changelog file on release branch:

```bash
python tools/gitlog2changelog.py
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
1dev
2019
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
```

### Upload source code tarball to OSGeo servers

```bash
# Store the source tarball (twice) in (use scp -p FILES grass:):
SERVER1=grass.osgeo.org
SERVER1DIR=/var/www/grass/grass-cms/grass$MAJOR$MINOR/source/
SERVER2=upload.osgeo.org
SERVER2DIR=/osgeo/download/grass/grass$MAJOR$MINOR/source/
echo $SERVER1:$SERVER1DIR
echo $SERVER2:$SERVER2DIR

# upload along with associated files:
scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html SUBMITTING neteler@$SERVER1:$SERVER1DIR

scp -p grass-$VERSION.* AUTHORS COPYING ChangeLog_$VERSION.gz \
  INSTALL REQUIREMENTS.html SUBMITTING neteler@$SERVER2:$SERVER2DIR

# Only at full release!
# generate link to "latest" source code
ssh neteler@$SERVER1 "cd $SERVER1DIR ; rm -f grass-$MAJOR.$MINOR-latest.tar.gz"
ssh neteler@$SERVER1 "cd $SERVER1DIR ; ln -s grass-$VERSION.tar.gz grass-$MAJOR.$MINOR-latest.tar.gz"

# verify
echo "https://$SERVER1/grass$MAJOR$MINOR/source/"

# update winGRASS related files: Update the winGRASS version
vim grass-addons/tools/wingrass-packager/grass_packager_release.bat
vim grass-addons/tools/wingrass-packager/grass_addons.sh
vim grass-addons/tools/wingrass-packager/grass_copy_wwwroot.sh
vim grass-addons/tools/wingrass-packager/cronjob.sh       # major/minor release only

# update addons - major/minor release only
vim grass-addons/tools/addons/grass-addons-publish.sh
vim grass-addons/tools/addons/grass-addons-build.sh
vim grass-addons/tools/addons/grass-addons.sh
```

Release is done.

### Advertise new release

#### Update CMS web site to show new version

- News section
- <https://grass.osgeo.org/download/software/>
- <https://grass.osgeo.org/download/software/sources/>
- <https://grass.osgeo.org/download/software/linux/>
- <https://grass.osgeo.org/home/history/releases/>

TODO: git tags

- <https://grass.osgeo.org/development/svn/svn-tags/> (add tag): echo $RELEASETAG

#### Write announcement and publish it

- store in trac:
    - <https://trac.osgeo.org/grass/wiki/Release/7.8.x-News>
    - <https://trac.osgeo.org/grass/wiki/Grass7/NewFeatures78>  <- add content
      of major changes only
- update version in <https://grasswiki.osgeo.org/wiki/GRASS-Wiki>
- ~~store in Web as announces/announce_grass$MAJOR$MINOR$RELEASE.html <- how?
  with protected PHP upload page?~~ (dropped since CMS)

#### Only when new major release

- update cronjob 'cron_grass_HEAD_src_snapshot.sh' on grass.osgeo.org to next
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
        - <https://trac.osgeo.org/grass/query?status=assigned&status=new&status=reopened&milestone=7.8.0&milestone=7.8.1&group=status&col=id&col=summary&col=owner&col=type&col=priority&col=component&col=version&order=priority>
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
     set PATCH=0RC1
```

- Update addons (grass_addons.sh) rules, eg.

```
     compile $GIT_PATH/grass7 $GISBASE_PATH/grass780RC1   $ADDON_PATH/grass780RC1/addons
```

- Modify grass_copy_wwwroot.sh accordingly, eg.

```
     copy_addon 781RC1 7.8.1RC1
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
- DebianGIS: <debian-gis@lists.debian.org> - send only small note
- FreeGIS: <freegis-list@intevation.de>
- Geowanking: <geowanking@geowanking.org>
- OSGeo.org: <news_item@osgeo.org>

Via Email:

- info@osgeo.org
- <http://www.gis-news.de/>  (franz-josef.behr@gismngt.de)
- mfeilner@linuxnewmedia.de
- info@harzer.de
- editor-geo@geoconnexion.com

Via Web:

- <http://linuxtoday.com/contribute.php3>
- <https://joinup.ec.europa.eu/software/grassgis/home> (submit news, MN)
- <http://www.macnn.com/contact/newstips/1>
- <http://www10.giscafe.com/submit_material/submit_options.php#Press> (MN) -->
  Press releases
- <http://www.directionsmag.com/pressreleases/> (News -> Submit Press Release)
- <http://directory.fsf.org/wiki/GRASS_%28Geographic_Resources_Analysis_Support_System%29>
- <https://www.linux-apps.com/p/1128004/edit/> (MN)
- <https://news.eoportal.org/web/eoportal/share-your-news> (MN) -> Share your
  news with the EO community
- <https://www.heise.de/download/product/grass-gis-7105> (update, MN)
- See also: <https://grass.osgeo.org/wiki/Contact_Databases>
- ... anywhere else? Please add here.
