# Branching How-To

## Assumptions

Given the creation of a new release branch will typically happen right
before RC1 of a new release series, please see assumptions in the
`howto_release.md` document.

## Create a New Branch

Given how the branch protection rules are set up and work,
you need to temporarily bypass protection against merge commits on branches.
(We don't want any new merge commits. However, there are merge commits
from the past and they prevent the creation of a new branch when the rules are
applied.)

To bypass, go to _Settings > Rules > Rulesets > Rules for release branches_.
Press _Add bypass_ and add the team or user who is creating the branch (e.g.,
"Repository admin"). Then _Save_ the new settings.

Use GitHub web interface to create a new branch:

1. Go to _branches_ (<https://github.com/OSGeo/grass/branches>).
2. Copy the name of one of the existing branches.
3. Click _New branch_.
4. Paste the name of the existing branch.
5. Modify the name.
6. Click _Create branch_.

As an alternative to creation in GitHub, you can
[create a new branch using command line](#create-in-command-line).

Note down the latest commit hash on the branch to record it in the
[release history overview](https://grass.osgeo.org/about/history/releases/).
The instructions for updating the website come later in the procedure.

Now remove the bypass in _Settings_ and _Save_.

## Check the Version

```bash
git fetch upstream
git switch releasebranch_8_5
```

## Increase Version on the main Branch

The version number needs to be increased on the main branch.

```bash
git switch main
git fetch upstream
git rebase upstream/main
```

Update the version in the source code (use `minor` or `major`):

```bash
./utils/update_version.py minor
```

If you are using a clone you use for building GRASS,
clean up (`make distclean`) your GRASS build to remove
the now outdated generated version numbers.
(You don't need to build GRASS if you have a fresh clone.)

Search for all other mentions of the last few versions to see
if they need to be updated, for example:

```bash
grep --exclude-dir=.git -IrnE "[^0-9^a-z]8[\._][0-9][^0-9]"
grep --exclude-dir=.git -IrnE "grass8.?[0-9]"
```

After the check and update, commit

```bash
git switch -c update-version
git add -p
./utils/update_version.py suggest
git commit -m "version: ..."
git push --set-upstream origin update-version
```

Create a PR and review and merge it as soon as possible to avoid having
the wrong version on the branch in case other PRs need to be merged.

## Server Updates

On grass.osgeo.org (grasslxd), new version directories need to be created:

```bash
cd /var/www/code_and_data/
VER=grass86
mkdir -p ${VER}/manuals \
                ${VER}/source/snapshot \
                ${VER}/binary/mswindows/native \
                ${VER}/binary/linux/snapshot
```

## Updates of grass-addon Repo

* Add new branch into the build matrix ("jobs") in
[`.github/workflows/ci.yml`](https://github.com/OSGeo/grass-addons/blob/grass8/.github/workflows/ci.yml).

## Website Updates

Add the branch creation to the release history (use the commit hash you saved earlier):

<https://github.com/OSGeo/grass-website/blob/master/content/about/history/releases.md>

## Additional Notes

### Making Changes in General

If you make changes, commit them:

```bash
git commit -m "version: ..."
git push
```

(You can directly push to release branches.)

### Create in Command Line

Get the latest main branch:

```bash
git switch main
git fetch upstream
git rebase upstream/main
```

Create the branch:

```bash
git switch -c releasebranch_8_5
```

Push the version to the upstream repo:

```bash
git push upstream
