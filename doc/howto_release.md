# How to release GRASS GIS binaries and source code

## Assumptions

- You have communicated with other developers, e.g., through grass-dev mailing list.
- You have communicated with a development coordinator.
- You have evaluated status of issues and PRs associated with the relevant milestone.
- You have already cloned the repo with Git.
- Your own fork is the remote called "origin".
- The OSGeo repo is the remote called "upstream".
- You don't have any local un-pushed or un-committed changes.
- You are using Bash or a similar shell.

*Note: Some later steps in this text are to be done by the development coordinator
(currently Markus Neteler and Martin Landa) due to needed logins.*

## Prepare the local repo

Update your remotes and switch to branch:

```bash
git fetch --all --prune && git checkout releasebranch_8_2
```

Confirm that you are on the right branch and have no local changes:

```bash
# Should show no changes:
git status
# Should give no output at all:
git diff
git diff --staged
git log upstream/releasebranch_8_2..HEAD
# Should give the same as last commits visible on GitHub:
git log --max-count=5
```

Now you can merge (or rebase) updates from the remote your local branch
and optionally update your own fork:

```bash
git merge upstream/releasebranch_8_2 && git push origin releasebranch_8_2
```

Use `git log` and `git show` to verify the result:

```bash
git log --max-count=5
git show
```

Any time later, you can use `git log` and `git show` to see the latest
commits and the last commit including the changes.

## Update VERSION file to release version number

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

## Create variables

For convenience, create Bash variables with the version update script:

```bash
# Get VERSION and TAG as variables.
eval $(./utils/update_version.py status --bash)
```

Version and tag are the same for all releases:

```bash
echo "$VERSION"
echo "$TAG"
```

If in doubt, run the script without `eval $(...)` to see all the variables created.

## Create release tag

The tag is created locally, while the release draft is created automatically by
GitHub Actions and can be later edited on GitHub. For background on GitHub releases,
see: <https://help.github.com/en/articles/creating-releases>.

### Tag release

Before creating the tag, it is a good idea to see if the CI jobs are not failing.
Check on GitHub or use GitHub CLI:

```bash
gh run list --branch releasebranch_8_2
```

Create an annotated tag (a lightweight tag is okay too, but there is more metadata
stored for annotated tags including a date):

```bash
git tag $TAG -a -m "GRASS GIS 8.2.0RC1"
```

Now push the tag upstream - this will trigger the automated workflows linked to tags:

```bash
git push upstream $TAG
```

If the job fails, open an issue and see what you can do manually.

### Create release notes

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

### Modify the release draft

After the automated job completes, a new release draft will be available in the GitHub
web interface. You can copy-paste the created release notes to GitHub and further modify as needed.

Older release description may or may not be a good inspiration:
<https://github.com/OSGeo/grass/releases>.

If RC, mark it as a pre-release, check:

```
[x] This is a pre-release
```

Save the modified draft, but do not publish the release yet.

## Changelog file for upload

There is also a large changelog file we produce and publish,
create it with a script (it takes several minutes to complete):

```bash
python3 utils/gitlog2changelog.py
mv ChangeLog ChangeLog_$VERSION
head ChangeLog_$VERSION
gzip ChangeLog_$VERSION
```

## Reset include/VERSION file to git development version

Use a dedicated script to edit the VERSION file.

After an RC, switch to development version:

```bash
./utils/update_version.py dev
```

After a (final) release, switch to development version for the next micro, minor, or major
version, e.g., for micro version, use:

```bash
./utils/update_version.py micro
```

Use _major_ and _minor_ operations for the other version updates.
Use `--help` for details about the options.

Commit with the suggested commit message and push, e.g.:

```bash
git show
git commit include/VERSION -m "version: Back to 8.2.0dev"
git push upstream
```

## Get the source code tarball

Fetch a tarball from GitHub we also publish on OSGeo servers:

```bash
wget https://github.com/OSGeo/grass/archive/${VERSION}.tar.gz -O grass-${VERSION}.tar.gz
md5sum grass-${VERSION}.tar.gz > grass-${VERSION}.md5sum
```

## Upload source code tarball to OSGeo servers

Note: servers 'osgeo8-grass' and 'osgeo7-download' only reachable via
      jumphost (managed by OSGeo-SAC) - see https://wiki.osgeo.org/wiki/SAC_Service_Status#grass

```bash
# Store the source tarball (twice) in (use scp -p FILES grass:):
USER=neteler
SERVER1=osgeo8-grass
SERVER1DIR=/var/www/code_and_data/grass$MAJOR$MINOR/source/
SERVER2=osgeo7-download
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

## Update addon builders

Add the new version to repos which build or test addons:

- https://github.com/OSGeo/grass-addons/blob/grass8/.github/workflows/ci.yml (currently new branches only)
- https://github.com/landam/wingrass-maintenance-scripts/blob/master/grass_addons.sh (add new release related line)

## Close milestone

For a (final) release (not release candidate), close the related milestone at
<https://github.com/OSGeo/grass/milestones>.
If there are any open issues or PRs, move them to another milestone
in the milestone view (all can be moved at once).

## Publish the release

When the above is done and the release notes are ready, publish the release at
<https://github.com/OSGeo/grass/releases>.

Release is done.

## Create entries for the new release

### Trac Wiki release page

Add entry in https://trac.osgeo.org/grass/wiki/Release

### Update Hugo web site to show new version

For a (final) release (not release candidate), write announcement and publish it:
- News section, https://github.com/OSGeo/grass-website/tree/master/content/news

Software pages:
- Linux: https://github.com/OSGeo/grass-website/blob/master/content/download/linux.en.md
- Windows: https://github.com/OSGeo/grass-website/blob/master/content/download/windows.en.md
- Mac: https://github.com/OSGeo/grass-website/blob/master/content/download/mac.en.md
- Releases: https://github.com/OSGeo/grass-website/blob/master/content/about/history/releases.md
- Wiki: https://grasswiki.osgeo.org/wiki/GRASS-Wiki


### Only in case of new major release

- update cronjob '[cron_grass8_main_src_snapshot.sh](https://github.com/OSGeo/grass-addons/tree/grass8/utils/cronjobs_osgeo_lxd/)' on grass.osgeo.org to next
  but one release tag for the differences
- wiki updates, only when new major release:
    - {{cmd|xxxx}} macro: <https://grasswiki.osgeo.org/wiki/Template:Cmd>
    - update last version on main page
- Add trac Wiki Macro definitions for manual pages G8X:modulename
    - Edit: <https://trac.osgeo.org/grass/wiki/InterMapTxt>

## Packaging notes

### WinGRASS notes

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

### Ubuntu Launchpad notes

- Create milestone and release: <https://launchpad.net/grass/+series>
- Upload tarball for created release

### Other notes

- <https://trac.osgeo.org/grass/wiki/BuildHints>
    - <https://trac.osgeo.org/grass/wiki/DebianUbuntuPackaging>
    - <https://trac.osgeo.org/grass/wiki/CompileOnWindows>

## Tell others about release

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
