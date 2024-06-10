# Branching How-To

## Assumptions

Given the creation of a new release branch will typically happen right
before RC1 of a new release series, please see assumptions in the
`howto_release.md` document.

## Create a New Branch

Given how the branch protection rules are set up and work,
you need to bypass protection againts merge commits on branches
(we don't want any new ones, but there are existing from the past
which prevet creation of a new branch).
To bypass, go to _Settings > Rules > Rulesets > Rules for release branches_.
Press _Add bypass_ and add the team or user who is creating the branch.

Use GitHub web interface to create a new branch:

1. Go to _branches_.
2. Copy name of one of the existing branches.
3. Click _New branch_.
4. Paste name of the existing branch.
5. Modify the name.
6. Click _Create branch_.

As an alterntaive to creation in GitHub, you can create a new branch using
command line (instructions included at the end of the document).

Note down the latest commit hash on the branch which will be recorded in the
[release history overview](https://grass.osgeo.org/about/history/releases/).
The instructions for updating the website come later in the procedure.

Remove the bypass in _Settings_.

## Check the Version

```bash
git fetch upstream
git switch releasebranch_8_4
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

If you are using a clone you use for building GRASS GIS,
clean up (`make distclean`) your GRASS GIS build to remove
the now outdated generated version numbers.
(You don't need to build GRASS GIS if you have a fresh clone.)

Search for all other mentions of last few versions to see if they need to be updated,
for example:

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
git push
```

Create a PR and review and merge it as soon as possible to avoid having
the wrong version on the branch in case other PRs need to be merged.

## Server Updates

In the grass-addons repo:

* Add new branch into [`.github/workflows/ci.yml`](https://github.com/OSGeo/grass-addons/blob/grass8/.github/workflows/ci.yml).

## Website Updates

Add the branch creation to the following history:

<https://github.com/OSGeo/grass-website/blob/master/content/about/history/releases.md>

## Additional Notes

### Making Changes in General

If you make changes, commit them:

```bash
git commit -m "version: ..."
git push
```

(You can directly push to release branches.)

### Create by Command Line

Get the latest main branch:

```bash
git switch main
git fetch upstream
git rebase upstream/main
```

Create the branch:

```bash
git switch -c releasebranch_8_3
```

Push the version to the upstream repo:

```bash
git push upstream
