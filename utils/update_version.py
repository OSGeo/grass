#!/usr/bin/env python3

"""Update VERSION file to release and development versions"""

import sys
import datetime
from types import SimpleNamespace

import argparse


def read_version_file():
    """Return version file content as object instance with attributes"""
    with open("include/VERSION", encoding="utf-8") as file:
        lines = file.read().splitlines()
    return SimpleNamespace(
        major=lines[0], minor=lines[1], micro=lines[2], year=lines[3]
    )


def write_version_file(major, minor, micro, year):
    """Write version file content from object instance with attributes"""
    with open("include/VERSION", "w", encoding="utf-8") as file:
        file.write(f"{major}\n")
        file.write(f"{minor}\n")
        file.write(f"{micro}\n")
        file.write(f"{year}\n")


def is_int(value):
    """Return True if the *value* represents an integer, False otherwise"""
    try:
        int(value)
        return True
    except ValueError:
        return False


def this_year():
    """Return current year"""
    return datetime.date.today().year


def construct_version(version_info):
    """Construct version string from version info"""
    return f"{version_info.major}.{version_info.minor}.{version_info.micro}"


def suggest_commit_from_version_file(action):
    """Using information in the version file, suggest a commit message"""
    version_file = read_version_file()
    suggest_commit(action, construct_version(version_file))


def suggest_commit(action, version):
    """Suggest a commit message for action and version"""
    print("message: Use the provided title as a commit message")
    print(f"title: 'version: {action.capitalize()} {version}'")


def release_candidate(args):
    """Switch to RC"""
    version_file = read_version_file()
    micro = version_file.micro
    if micro.endswith("dev"):
        micro = micro[:-3]
        if not micro:
            sys.exit("Creating RC from a dev micro without number is not possible")
        micro = f"{micro}RC{args.number}"
    else:
        sys.exit(
            "Creating RC from a non-dev VERSION file "
            f"with micro '{micro}' is not possible"
        )
    write_version_file(
        major=version_file.major,
        minor=version_file.minor,
        micro=micro,
        year=this_year(),
    )
    suggest_commit_from_version_file("mark")


def release(_unused):
    """Switch to release version"""
    version_file = read_version_file()
    micro = version_file.micro
    if micro.endswith("dev"):
        micro = micro[:-3]
        if not micro:
            micro = 0
        micro = f"{micro}"
    else:
        sys.exit("Creating a release from a non-dev VERSION file is not possible")
    write_version_file(
        major=version_file.major,
        minor=version_file.minor,
        micro=micro,
        year=this_year(),
    )
    suggest_commit_from_version_file("mark")


def update_micro(_unused):
    """Update to next micro version"""
    version_file = read_version_file()
    micro = version_file.micro
    if micro == "dev":
        sys.exit("No micro version increases with development-only versions.")
        # We could also add micro version when not present, but requested with:
        # micro = "0dev"
    elif micro.endswith("dev"):
        sys.exit(f"Already dev with micro '{micro}'. Release first before update.")
    elif is_int(micro):
        micro = int(version_file.micro) + 1
        micro = f"{micro}dev"
    else:
        if "RC" in micro:
            sys.exit(
                f"Updating micro for RC '{micro}' is not possible. "
                "Release first before update."
            )
        sys.exit(f"Unknown micro version in VERSION file: '{micro}'")
    write_version_file(
        major=version_file.major,
        minor=version_file.minor,
        micro=micro,
        year=this_year(),
    )
    suggest_commit_from_version_file("start")


def update_minor(args):
    """Update to next minor version"""
    version_file = read_version_file()
    micro = version_file.micro
    minor = int(version_file.minor)
    if args.dev:
        if not minor % 2:
            sys.exit(
                "Updating to a development-only version "
                f"from an even minor version '{minor}' is not possible"
            )
        minor += 2
    else:
        minor += 1
    if micro.endswith("dev"):
        if minor % 2:
            # Odd is development-only, never released and without micro version.
            micro = "dev"
        else:
            # Even will be released, so adding micro version.
            micro = "0dev"
    else:
        sys.exit("Updating version from a non-dev VERSION file is not possible")
    write_version_file(
        major=version_file.major, minor=minor, micro=micro, year=this_year()
    )
    suggest_commit_from_version_file("start")


def update_major(_unused):
    """Update to next major version"""
    version_file = read_version_file()

    micro = version_file.micro
    if micro.endswith("dev"):
        micro = "0dev"
    else:
        sys.exit("Updating version from a non-dev VERSION file is not possible")
    minor = 0
    major = int(version_file.major) + 1
    write_version_file(major=major, minor=minor, micro=micro, year=this_year())
    suggest_commit_from_version_file("start")


def back_to_dev(_unused):
    """Switch version to development state"""
    version_file = read_version_file()
    micro = version_file.micro
    if "RC" in micro:
        micro = micro.split("RC")[0]
        micro = f"{micro}dev"
        action = "back to"
    elif is_int(micro):
        micro = int(micro) + 1
        micro = f"{micro}dev"
        action = "start"
    else:
        if micro.endswith("dev"):
            sys.exit(f"Already dev with micro '{micro}'")
        sys.exit(
            "Can switch to dev only from release or RC VERSION file, "
            f"not from micro '{micro}'"
        )
    write_version_file(
        major=version_file.major,
        minor=version_file.minor,
        micro=micro,
        year=this_year(),
    )
    suggest_commit_from_version_file(action)


def status(_unused):
    """Print VERSION file and today's date"""
    version_file = read_version_file()
    print(f"today: {datetime.date.today().isoformat()}")
    print(f"year: {version_file.year}")
    print(f"major: {version_file.major}")
    print(f"minor: {version_file.minor}")
    print(f"micro: {version_file.micro}")
    print(f"version: {construct_version(version_file)}")


def main():
    """Translate sub-commands to function calls"""
    parser = argparse.ArgumentParser(
        description="Update VERSION file using the specified action.",
        epilog="Run in the root directory to access the VERSION file.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparser = subparsers.add_parser(
        "rc", help="switch to release candidate (no dev suffix)"
    )
    subparser.add_argument(
        "number", type=int, help="RC number (number sequence not checked)"
    )
    subparser.set_defaults(func=release_candidate)

    subparser = subparsers.add_parser(
        "dev", help="switch to development state (attaches dev suffix)"
    )
    subparser.set_defaults(func=back_to_dev)

    subparser = subparsers.add_parser(
        "release", help="switch to release version (no dev suffix)"
    )
    subparser.set_defaults(func=release)

    subparser = subparsers.add_parser(
        "major", help="increase major (X.y.z) version (attaches dev suffix)"
    )
    subparser.set_defaults(func=update_major)

    subparser = subparsers.add_parser(
        "minor", help="increase minor (x.Y.z) version (uses dev in micro)"
    )
    subparser.add_argument(
        "--dev", action="store_true", help="increase development-only version"
    )
    subparser.set_defaults(func=update_minor)

    subparser = subparsers.add_parser(
        "micro", help="increase micro (x.y.Z, aka patch) version (attaches dev suffix)"
    )
    subparser.set_defaults(func=update_micro)

    subparser = subparsers.add_parser(
        "status", help="show status of VERSION file as YAML"
    )
    subparser.set_defaults(func=status)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
