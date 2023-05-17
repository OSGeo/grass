# Branching How-To

Use GitHub web interface to create a new branch:

Go to _branches_ Click _New branch_ Click _Create branch_

## Check the Version

git fetch upstream
git switch releasebranch_8_3

commit
push

## Increase Version on the main Branch

The version number needs to be increased on the main branch.

```sh
git switch main
git fetch upstream
git rebase upstream/main
```

```sh
./utils/update_version.py minor
```

branch
PR

Search for all mentions of all 

## Server Updates

## Website Updates



## Create by Command Line

```sh
git switch main
git fetch upstream
git rebase upstream/main
```

```sh
git switch -c releasebranch_8_3
```
