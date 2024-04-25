# RFC: Release Policy

Author: Vaclav Petras

Status: Draft

## Goal

The goal of this document is to define when releases should be created and what types releases should be created.

## Frequency

One major or minor release should be released once a year.
The may be multiple micro releases per year.

## Maitanance

We have only one actively updated, maintained, and supported release series.

Once a new major or minor version is released, the old release series goes to
a low maintenance mode and no further released are planned.
Low maintanance series are no longer actively updated, but can be updated on demand,
i.e., if you submit a patch to fix a bug, we will likely accept it and
create a new release when there is enough changes accumulated
or support you in creating the release as a temporary release manager.

## Additional Types of Releases

For some platfroms we have daily builds of development version from main.

## Relation to Other Documents

* Release procedure covers branching and creation of pre-releases.
