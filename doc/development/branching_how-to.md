# Branching How-To

## Assumptions

Given the creation of a new release branch will typically happen right
before RC1 of a new release series, please see assumptions in the
`howto_release.md` document.

## Create a New Branch

Use GitHub web interface to create a new branch:

1. Go to _branches_.
2. Copy name of one of the existing branches.
3. Click _New branch_.
4. Paste name of the existing branch.
5. Modify the name.
6. Click _Create branch_.

Note down the commit hash which will be recorded in the [release history overview](https://grass.osgeo.org/about/history/releases/).
Alternatively, see the instructions below to create a new branch using command line.

## Check the Version

```bash
git fetch upstream
git switch releasebranch_8_4
```

If you make changes, commit them:

```bash
git commit -m "version: ..."
git push
```

(You can directly push to release branches.)

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

If you are using a clone you also use for building GRASS GIS,
rebuild GRASS GIS to update the generated version numbers.
(You don't need to build GRASS GIS if you have a fresh clone.)

Search for all other mentions of last few versions to see if they need to be updated.

```bash
git switch -c update-version
git commit -m "version: ..."
git push
```

Create a PR and review and merge it as soon as possible to avoid having
the wrong version on the branch in case other PRs need to be merged.

## Server Updates

(Here go things like updates to grass-addons repo.)

## Website Updates

Add the branch creation to the following history:

<https://github.com/OSGeo/grass-website/blob/master/content/about/history/releases.md>

## Create by Command Line

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
