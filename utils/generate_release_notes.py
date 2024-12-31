#!/usr/bin/env python3

"""Generate release notes using git log or GitHub API

Needs PyYAML, Git, and GitHub CLI.
"""

import argparse
import csv
import itertools
import json
import random
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

import requests
import yaml

PRETTY_TEMPLATE = (
    "  - hash: %H%n"
    "    author_name: %aN%n"
    "    author_email: %aE%n"
    "    date: %ad%n"
    "    message: |-%n      %s"
)
CONFIG_DIRECTORY = Path("utils")


def remove_excluded_changes(changes, exclude):
    """Return a list of changes with excluded changes removed"""
    result = []
    for change in changes:
        include = True
        for expression in exclude["regexp"]:
            if re.match(expression, change):
                include = False
                break
        if include:
            result.append(change)
    return result


def round_down_to_five(value):
    """Round down to the nearest multiple of five"""
    base = 5
    return value - (value % base)


def split_to_categories(changes, categories):
    """Return dictionary of changes divided into categories

    *categories* is a list of dictionaries (mappings) with
    keys title and regexp.
    """
    by_category = defaultdict(list)
    for change in changes:
        added = False
        for category in categories:
            if re.match(category["regexp"], change):
                by_category[category["title"]].append(change)
                added = True
                break
        if not added:
            by_category["Other Changes"].append(change)
    return by_category


def print_section_heading_2(text, file=None):
    print(f"## {text}\n", file=file)


def print_section_heading_3(text, file=None):
    print(f"### {text}\n", file=file)


def print_category(category, changes, file=None):
    """Print changes for one category from dictionary of changes

    If *changes* don't contain a given category, nothing is printed.
    """
    items = changes.get(category, None)
    if not items:
        return
    print_section_heading_3(category, file=file)
    bot_file = Path("utils") / "known_bot_names.txt"
    known_bot_names = bot_file.read_text().splitlines()
    visible = []
    hidden = []
    overflow = []
    max_section_length = 25
    for item in sorted(items):
        # Relies on author being specified after the last "by".
        author = item.rsplit(" by ", maxsplit=1)[-1]
        # Relies on author being specified as username.
        if " " in author:
            author = author.split(" ", maxsplit=1)[0]
        author = author.removeprefix("@")
        if author in known_bot_names or author.endswith("[bot]"):
            hidden.append(item)
        elif len(visible) > max_section_length:
            overflow.append(item)
        else:
            visible.append(item)
    for item in visible:
        print(f"* {item}", file=file)
    if hidden or overflow:
        print("\n<details>")
        print(" <summary>Show more</summary>\n")
        for item in itertools.chain(overflow, hidden):
            print(f"  * {item}", file=file)
        print("\n</details>")
    print()


def print_by_category(changes, categories, file=None):
    """Print changes by categories from dictionary of changes"""
    for category in categories:
        print_category(category["title"], changes, file=file)
    print_category("Other Changes", changes, file=file)


def binder_badge(tag):
    """Get mybinder Binder badge from a given tag, hash, or branch"""
    binder_image_url = "https://mybinder.org/badge_logo.svg"
    binder_url = f"https://mybinder.org/v2/gh/OSGeo/grass/{tag}?urlpath=lab%2Ftree%2Fdoc%2Fnotebooks%2Fjupyter_example.ipynb"  # noqa
    return f"[![Binder]({binder_image_url})]({binder_url})"


def print_support(file=None):
    url = "https://opencollective.com/grass/tiers/supporter/all.json"
    response = requests.get(url=url, timeout=7)
    data = response.json()
    if data:
        print_section_heading_3("Monthly Financial Supporters", file=file)
        random.shuffle(data)
        supporters = []
        for member in data:
            supporters.append(f"""[{member['name']}]({member['profile']})""")
        print(", ".join(supporters))
        print()


def adjust_after(lines):
    """Adjust new contributor lines in the last part of the generated notes"""
    bot_file = Path("utils") / "known_bot_names.txt"
    known_bot_names = bot_file.read_text().splitlines()
    new_lines = []
    for line in lines:
        if line.startswith("* @"):
            unused, username, text = line.split(" ", maxsplit=2)
            username = username.replace("@", "")
            if username in known_bot_names:
                continue
            output = subprocess.run(
                ["gh", "api", f"users/{username}"],
                capture_output=True,
                text=True,
                check=True,
            ).stdout
            name = json.loads(output)["name"]
            if name and name != username:
                line = f"* {name} (@{username}) {text}"
        new_lines.append(line)
    return new_lines


def print_notes(
    start_tag, end_tag, changes, categories, before=None, after=None, file=None
):
    """Print notes from given inputs

    *changes* is a list of strings. It will be sorted and ordered by category
    internally by this function.
    """
    num_changes = round_down_to_five(len(changes))
    print(
        f"The GRASS GIS {end_tag} release provides more than "
        f"{num_changes} improvements and fixes "
        f"with respect to the release {start_tag}.\n"
    )

    if before:
        print(before)
    print_section_heading_2("Highlights", file=file)
    print("* _Put handcrafted list of 2-15 items here._\n")
    print_section_heading_2("New Addon Tools", file=file)
    print(
        "* _Put here a list of new addos since last release "
        "or delete the section if there are none._\n"
    )
    print_support(file=file)
    print_section_heading_2("What's Changed", file=file)
    changes_by_category = split_to_categories(changes, categories=categories)
    print_by_category(changes_by_category, categories=categories, file=file)
    if after:
        print(after)
        print()
    print(binder_badge(end_tag))


def notes_from_gh_api(start_tag, end_tag, branch, categories, exclude):
    """Generate notes from GitHub API"""
    text = subprocess.run(
        [
            "gh",
            "api",
            "repos/OSGeo/grass/releases/generate-notes",
            "-f",
            f"previous_tag_name={start_tag}",
            "-f",
            f"tag_name={end_tag}",
            "-f",
            f"target_commitish={branch}",
        ],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    body = json.loads(text)["body"]

    lines = body.splitlines()
    start_whats_changed = lines.index("## What's Changed")
    end_whats_changed = lines.index("", start_whats_changed)
    raw_changes = lines[start_whats_changed + 1 : end_whats_changed]
    changes = []
    for change in raw_changes:
        if change.startswith(("* ", "- ")):
            changes.append(change[2:])
        else:
            changes.append(change)
    changes = remove_excluded_changes(changes=changes, exclude=exclude)
    after = adjust_after(lines[end_whats_changed + 1 :])
    print_notes(
        start_tag=start_tag,
        end_tag=end_tag,
        changes=changes,
        before="\n".join(lines[:start_whats_changed]),
        after="\n".join(after),
        categories=categories,
    )


def csv_to_dict(filename, key, value):
    """Read a CSV file as a dictionary"""
    result = {}
    with open(filename, encoding="utf-8", newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            result[row[key]] = row[value]
    return result


def notes_from_git_log(start_tag, end_tag, categories, exclude):
    """Generate notes from git log"""
    text = subprocess.run(
        ["git", "log", f"{start_tag}..{end_tag}", f"--pretty=format:{PRETTY_TEMPLATE}"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    commits = yaml.safe_load(text)
    if not commits:
        msg = "No commits retrieved from git log (try different tags)"
        raise RuntimeError(msg)

    svn_name_by_git_author = csv_to_dict(
        CONFIG_DIRECTORY / "svn_name_git_author.csv",
        key="git_author",
        value="svn_name",
    )
    github_name_by_svn_name = csv_to_dict(
        CONFIG_DIRECTORY / "svn_name_github_name.csv",
        key="svn_name",
        value="github_name",
    )
    github_name_by_git_author_file = CONFIG_DIRECTORY / "git_author_github_name.csv"
    github_name_by_git_author = csv_to_dict(
        github_name_by_git_author_file,
        key="git_author",
        value="github_name",
    )

    lines = []
    unknow_authors = []
    for commit in commits:
        if commit["author_email"].endswith("users.noreply.github.com"):
            github_name = commit["author_email"].split("@")[0]
            if "+" in github_name:
                github_name = github_name.split("+")[1]
            github_name = f"@{github_name}"
        else:
            # Emails are stored with @ replaced by a space.
            email = commit["author_email"].replace("@", " ")
            git_author = f"{commit['author_name']} <{email}>"
            if git_author in github_name_by_git_author:
                github_name = github_name_by_git_author[git_author]
                github_name = f"@{github_name}"
            else:
                try:
                    svn_name = svn_name_by_git_author[git_author]
                    github_name = github_name_by_svn_name[svn_name]
                    github_name = f"@{github_name}"
                except KeyError:
                    github_name = git_author
                    unknow_authors.append((git_author, commit["message"]))
        lines.append(f"{commit['message']} by {github_name}")
    lines = remove_excluded_changes(changes=lines, exclude=exclude)
    print_notes(
        start_tag=start_tag,
        end_tag=end_tag,
        changes=lines,
        after=(
            "**Full Changelog**: "
            f"https://github.com/OSGeo/grass/compare/{start_tag}...{end_tag}"
        ),
        categories=categories,
    )
    processed_authors = []
    if unknow_authors:
        print(
            f"\n\nAuthors who need to be added to {github_name_by_git_author_file}:\n"
        )
        for author, message in unknow_authors:
            if author in processed_authors:
                continue
            print(f"{author} -- authored {message}")
            processed_authors.append(author)


def create_release_notes(args):
    """Create release notes based on parsed command line parameters"""
    end_tag = args.end_tag
    if not end_tag:
        # git log has default, but the others do not.
        end_tag = subprocess.run(
            ["git", "rev-parse", "--verify", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()

    config_file = CONFIG_DIRECTORY / "release.yml"
    config = yaml.safe_load(config_file.read_text(encoding="utf-8"))["notes"]

    if args.backend == "api":
        notes_from_gh_api(
            start_tag=args.start_tag,
            end_tag=end_tag,
            branch=args.branch,
            categories=config["categories"],
            exclude=config["exclude"],
        )
    else:
        notes_from_git_log(
            start_tag=args.start_tag,
            end_tag=end_tag,
            categories=config["categories"],
            exclude=config["exclude"],
        )


def main():
    """Parse command line arguments and create release notes"""
    parser = argparse.ArgumentParser(
        description="Generate release notes from git log or GitHub API.",
        epilog="Run in utils directory to access the helper files.",
    )
    parser.add_argument(
        "backend",
        choices=["log", "api", "check"],
        help="use git log or GitHub API (or check a PR title)",
    )
    parser.add_argument(
        "branch",
        help="needed for the GitHub API when tag does not exist (or a PR title)",
    )
    parser.add_argument("start_tag", help="old tag to compare against")
    parser.add_argument(
        "end_tag",
        help=(
            "new tag; "
            "if not created yet, "
            "an empty string for git log will use the current revision"
        ),
    )
    args = parser.parse_args()
    if args.backend == "check":
        config_file = Path("utils") / "release.yml"
        config = yaml.safe_load(Path(config_file).read_text(encoding="utf-8"))
        has_match = False
        for category in config["notes"]["categories"]:
            if re.match(category["regexp"], args.branch):
                has_match = True
                break
        for item in config["notes"]["exclude"]["regexp"]:
            if re.match(item, args.branch):
                has_match = True
                break
        if has_match:
            sys.exit(0)
        else:
            expressions = "\n".join(
                [category["regexp"] for category in config["notes"]["categories"]]
            )
            suggestions = "\n".join(
                [category["example"] for category in config["notes"]["categories"]]
            )
            sys.exit(
                f"Title '{args.branch}' does not fit into one of "
                f"the categories specified in {config_file}. "
                "Try to make it fit one of these regular expressions:\n"
                f"{expressions}\n"
                "Here are some examples:\n"
                f"{suggestions}"
            )
    try:
        create_release_notes(args)
    except subprocess.CalledProcessError as error:
        sys.exit(f"Subprocess '{' '.join(error.cmd)}' failed with: {error.stderr}")


if __name__ == "__main__":
    main()
