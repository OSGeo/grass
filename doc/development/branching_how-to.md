# Branching How-To

Use GitHub web interface to create a new branch:

Go to _branches_ Click _New branch_ Click _Create branch_

## Check the Version

```bash
git fetch upstream
git switch releasebranch_8_3
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

And do a PR.

## Server Updates

(Here go things like updates to grass-addons repo.)

## Website Updates

(Here go things like addition of branch creation to history.)

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
```
