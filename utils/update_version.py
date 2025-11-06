#!/usr/bin/env python3

"""Update VERSION file to release and development versions"""

import argparse
import datetime
import sys
from pathlib import Path
from types import SimpleNamespace


def read_version_file():
    """Return version file content as object instance with attributes"""
    lines = Path("include/VERSION").read_text(encoding="utf-8").splitlines()
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


def suggest_commit_from_version_file(action, tag):
    """Using information in the version file, suggest a commit message"""
    version_file = read_version_file()
    suggest_commit(action, construct_version(version_file), tag=tag)


def suggest_commit(action, version, tag):
    """Suggest a commit message for action and version"""
    print("read:")
    if tag:
        print("  user_message: Use the provided messages for the commit and the tag.")
        print("  note: Once all checks pass, you are expected to create a tag.")
    else:
        print("  user_message: Use the provided message for the commit")
    print("use:")
    print(f"  commit_message: 'version: {action} {version}'")
    if tag:
        print(f"  tag_message: 'GRASS {version}'")


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
    suggest_commit_from_version_file("GRASS", tag=True)


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
    suggest_commit_from_version_file("GRASS", tag=True)


def update_micro(_unused):
    """Update to next micro version"""
    version_file = read_version_file()
    micro = version_file.micro
    if micro == "dev":
        sys.exit("The micro version does not increase with development-only versions.")
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
    suggest_commit_from_version_file("Start", tag=False)


def update_minor(args):
    """Update to next minor version"""
    version_file = read_version_file()
    micro = version_file.micro
    minor = int(version_file.minor)
    minor += 1
    if micro.endswith("dev"):
        micro = "0dev"
    else:
        sys.exit("Updating version from a non-dev VERSION file is not possible")
    write_version_file(
        major=version_file.major, minor=minor, micro=micro, year=this_year()
    )
    suggest_commit_from_version_file("Start", tag=False)


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
    suggest_commit_from_version_file("Start", tag=False)


def back_to_dev(_unused):
    """Switch version to development state"""
    version_file = read_version_file()
    micro = version_file.micro
    if "RC" in micro:
        micro = micro.split("RC")[0]
        micro = f"{micro}dev"
        action = "Back to"
    elif is_int(micro):
        micro = int(micro) + 1
        micro = f"{micro}dev"
        action = "Start"
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
    suggest_commit_from_version_file(action, tag=False)


def status_as_yaml(version_info, today, version, tag):
    """Print VERSION file and today's date as YAML"""
    print(f"today: {today}")
    print(f"year: {version_info.year}")
    print(f"major: {version_info.major}")
    print(f"minor: {version_info.minor}")
    print(f"micro: {version_info.micro}")
    print(f"version: {version}")
    if tag:
        print(f"tag: {version}")


def status_as_bash(version_info, today, version, tag):
    """Print VERSION file and today's date as Bash eval variables"""
    print(f"TODAY={today}")
    print(f"YEAR={version_info.year}")
    print(f"MAJOR={version_info.major}")
    print(f"MINOR={version_info.minor}")
    print(f"MICRO={version_info.micro}")
    print(f"VERSION={version}")
    if tag:
        print(f"TAG={version}")


def status(args):
    """Print VERSION file and today's date"""
    version_info = read_version_file()
    today = datetime.date.today().isoformat()
    version = construct_version(version_info)
    tag = version if not version_info.micro.endswith("dev") else None
    if args.bash:
        status_as_bash(version_info=version_info, today=today, version=version, tag=tag)
    else:
        status_as_yaml(version_info=version_info, today=today, version=version, tag=tag)


def suggest_message(args):
    """Print suggestion for a commit message

    Assumes that the version file was changed, but not commited yet,
    but it does not check that assumption.

    This shows a wrong commit message if going back from RCs,
    but it is not likely this is needed because the suggestion
    will be part of the message for the switch and there is
    no other work to do afterwards except for the commit
    (unlike updating the version number).
    """
    version_info = read_version_file()
    if not version_info.micro.endswith("dev"):
        tag = construct_version(version_info)
        action = "GRASS"
    else:
        tag = None
        action = "Start"
    suggest_commit_from_version_file(action, tag=tag)


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
    subparser.set_defaults(func=update_minor)

    subparser = subparsers.add_parser(
        "micro", help="increase micro (x.y.Z, aka patch) version (attaches dev suffix)"
    )
    subparser.set_defaults(func=update_micro)

    subparser = subparsers.add_parser(
        "status", help="show status of VERSION file (as YAML by default)"
    )
    subparser.add_argument(
        "--bash", action="store_true", help="format as Bash variables for eval"
    )
    subparser.set_defaults(func=status)

    subparser = subparsers.add_parser(
        "suggest", help="suggest a commit message for new version"
    )
    subparser.set_defaults(func=suggest_message)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
