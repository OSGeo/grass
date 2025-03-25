# Guide to contributing on GitHub

This guide covers contributing to the GRASS GIS source,
specifically to the _main_ branch.
It assumes that you have some very basic knowledge of Git and GitHub.

## First time setup

1. Create an account on GitHub.
1. Install Git on your computer.
1. Set up Git with your name and email.
1. Fork the repository (by clicking the `Fork` button in the upper right corner
  of the GitHub interface).
1. Clone your fork (use HTTPS or SSH URL, here we will use HTTPS):

```bash
git clone https://github.com/your_GH_account/grass.git
```

1. Enter the directory:

```bash
cd grass/
```

1. Add the main GRASS GIS repository as "upstream" (use HTTPS URL):

```bash
git remote add upstream https://github.com/OSGeo/grass
```

1. Your remotes now should be "origin" which is your fork and "upstream" which
  is this main GRASS GIS repository. You can confirm that using:

```bash
git remote -v
```

1. You should see something like:

```bash
origin  https://github.com/your_GH_account/grass.git (fetch)
origin  https://github.com/your_GH_account/grass.git (push)
upstream  https://github.com/OSGeo/grass (fetch)
upstream  https://github.com/OSGeo/grass (push)

```

For the following workflow, it is important that
"upstream" points to the OSGeo/grass repository
and "origin" to your fork
(although generally, the naming is up to you).

## Update before creating a feature branch

Make sure your are using the _main_ branch to create the new branch:

```bash
git checkout main
```

Download updates from all branches from the _upstream_ remote:

```bash
git fetch upstream
```

Update your local _main_ branch to match the _main_ branch
  in the _upstream_ repository:

```bash
git rebase upstream/main
```

Notably, you should not make commits to your local main branch,
so the above is then just a simple update (and no actual
rebase or merge happens).

## Creating a new feature branch

Now you have updated your local _main_ branch, you can create a feature branch
based on it.

Create a new feature branch and switch to it:

```bash
git switch -c new-feature
```

## Committing

Add files to the commit (changed ones or new ones):

```bash
git add file1
git add file2
```

Commit the change. Write a meaningful commit message (first word is for example
the tool name):

```bash
git commit -m "tool: added a new feature doing X"
```

## Pushing changes to GitHub

Push your local feature branch to your fork:

```bash
git push origin new-feature
```

## Creating a pull request

When you push, GitHub will respond back in the command line to tell
you what URL to use to create a pull request (PR). You can follow that URL
or you can go any time later to your fork on GitHub, display the
branch `new-feature`, and GitHub will show you a button to create
a PR.

Alternatively, you can explore GitHub CLI tool (_gh_) which allows you
to do `git push` and create a PR in one step with `gh pr create -fw`.

## Guidelines for writing a meaningful pull request

A well-written pull request clearly conveys the purpose and impact of the
proposed changes.

### PR Title

The title should be descriptive and clearly summarize the main purpose or change
in the pull request. Start the title with the tool name or a
[keyword](https://github.com/OSGeo/grass/blob/main/utils/release.yml) (e.g.:
`tool name: Add functionality Y for Z`. Keep it short, i.e. aim for concise titles,
typically under 50-60 characters.

### PR Content

A pull request requires an abstract, change details, and more. When you create
the new PR, you are presented with a [template](https://github.com/OSGeo/grass/blob/main/.github/PULL_REQUEST_TEMPLATE/pull_request_template.md)
to help standardize the content.

## After creating a PR

GRASS GIS maintainers will now review your PR.
If needed, the maintainers will work with you to improve your changes.

Once the changes in the PR are ready to be accepted,
the maintainers will usually squash all your commits into one commit and merge it
to the _main_ branch.

Once the PR is merged, it is a good time to [update your
local _main_ branch](#update-before-creating-a-feature-branch) in order to get
the change you just contributed.

## Further notes

Here we cover common situations and how to deal with them:

### Update if you have local changes

Assumption is you are on the main branch and you are trying to update it.
If `git rebase` fails with `error: cannot rebase: You have unstaged changes...`,
then move your uncommitted local changes to "stash" using:

```bash
git stash
```

Now you can rebase:

```bash
git rebase upstream/main
```

Get the changes back from stash:

```bash
git stash pop
```

### Updating your PR from main

Updating your PR from the main branch is often referred to as rebasing a PR.
It comes into play in case there are changes in the main branch you need to
incorporate into your (feature or fix) branch before the PR can be merged,
you need to merge the upstream main into your branch:

```bash
git fetch upstream
git merge upstream/main
```

Git will ask you to edit the merge commit message, you can leave the default
and close the editor. You may run into a conflict,
in that case you need to resolve it.

### Testing PRs from other contributors

For testing other contributors' PRs, we recommend using
[GitHub CLI](https://cli.github.com/). To checkout a specific PR, run:

```bash
gh pr checkout <PR number>
```
