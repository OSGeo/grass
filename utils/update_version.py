#!/usr/bin/env python3

import sys
from types import SimpleNamespace

import argparse

def read_version_file():
    with open("include/VERSION") as file:
        lines = file.read().splitlines()
    return SimpleNamespace(
        major = lines[0],
        minor = lines[1],
        patch = lines[2],
        year = lines[3])

def write_version_file(major, minor, patch, year):
    with open("include/VERSION", "w") as file:
        file.write(f"{major}\n")
        file.write(f"{minor}\n")
        file.write(f"{patch}\n")
        file.write(f"{year}\n")


def is_int(x):
    try:
        int(x)
        return True
    except ValueError:
        return False


# sub-command functions
def release_candidate(args):
    version_file = read_version_file()
    patch = version_file.patch
    if patch.endswith("dev"):
        patch = patch[:-3]
        patch = f"{patch}RC{args.number}"
    else:
        sys.exit("Creating RC from a non-dev VERSION file is not possible")
        
    write_version_file(major=version_file.major, minor=version_file.minor, patch=patch, year=version_file.year)

def release(args):
    version_file = read_version_file()
    patch = version_file.patch
    if patch.endswith("dev"):
        patch = patch[:-3]
        if not patch:
            patch = 0
        patch = f"{patch}"
    else:
        sys.exit("Creating a release from a non-dev VERSION file is not possible")
        
    write_version_file(major=version_file.major, minor=version_file.minor, patch=patch, year=version_file.year)

def update_micro(args):
    version_file = read_version_file()
    patch = version_file.patch
    if patch == "dev":
        patch = "0dev"
    elif patch.endswith("dev"):
        sys.exit(f"Already dev with patch '{patch}'")
    else:
        sys.exit("Updating version from a non-dev VERSION file is not possible")
        
    write_version_file(major=version_file.major, minor=version_file.minor, patch=patch, year=version_file.year)


def update_minor(args):
    version_file = read_version_file()
    micro = version_file.patch
    minor = int(version_file.minor) + 1
    if micro.endswith("dev"):
        if minor % 2:
            # odd
            micro = "dev"
        else:
            # even
            micro = "0dev"
    else:
        sys.exit("Updating version from a non-dev VERSION file is not possible")
    write_version_file(major=version_file.major, minor=minor, patch=micro, year=version_file.year)

def update_major(args):
    version_file = read_version_file()
    
    micro = version_file.patch
    if micro.endswith("dev"):
        micro = "0dev"
    else:
        sys.exit("Updating version from a non-dev VERSION file is not possible")
    minor = 0
    major = int(version_file.major) + 1
    write_version_file(major=major, minor=minor, patch=micro, year=version_file.year)


def back_to_dev(args):
    version_file = read_version_file()
    patch = version_file.patch
    if "RC" in patch:
        patch = patch.split("RC")[0]
        patch = f"{patch}dev"
    elif is_int(patch):
        patch = int(patch) + 1
        patch = f"{patch}dev"
    else:
        if patch.endswith("dev"):
            sys.exit(f"Already dev with patch '{patch}'")
        sys.exit(f"Can switch to dev only from release or RC VERSION file, not from patch '{patch}'")
        
    write_version_file(major=version_file.major, minor=version_file.minor, patch=patch, year=version_file.year)


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    parser_foo = subparsers.add_parser('rc')
    parser_foo.add_argument('number', type=int)
    parser_foo.set_defaults(func=release_candidate)

    parser_bar = subparsers.add_parser('dev')
    parser_bar.set_defaults(func=back_to_dev)

    parser_bar = subparsers.add_parser('release')
    parser_bar.set_defaults(func=release)

    parser_bar = subparsers.add_parser('micro')
    parser_bar.set_defaults(func=update_micro)

    parser_bar = subparsers.add_parser('minor')
    parser_bar.set_defaults(func=update_minor)

    parser_bar = subparsers.add_parser('major')
    parser_bar.set_defaults(func=update_major)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
