#!/usr/bin/env python3

import csv
import subprocess


import yaml

pretty_template = (
    "  - hash: %H%n"
    "    author_name: %aN%n"
    "    author_email: %aE%n"
    "    date: %ad%n"
    "    message: |-%n      %s"
)


def round_down_to_five(x):
    base = 5
    return x - (x % base)


def main():
    start_tag = "8.0.1"
    end_tag = ""
    if not end_tag:
        # git log has default, but the others do not.
        end_tag = subprocess.run(
            ["git", "rev-parse", "--verify", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
    text = subprocess.run(
        ["git", "log", f"{start_tag}..{end_tag}", f"--pretty=format:{pretty_template}"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout
    commits = yaml.safe_load(text)
    num_maintenance_commits = 2
    num_commits = round_down_to_five(len(commits) - num_maintenance_commits)

    compare_url = f"https://github.com/OSGeo/grass/compare/{start_tag}...{end_tag}"
    print(
        f"The GRASS GIS {end_tag} release provides"
        f" more than [{num_commits} fixes and improvements]({compare_url})"
        f"with respect to the release {start_tag}.\n"
    )

    svn_name_by_git_author = {}

    with open("AUTHORS.txt", newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            svn_name_by_git_author[row["git_author"]] = row["svn_name"]

    github_name_by_svn_name = {}

    with open("svn2git_users.csv", newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            github_name_by_svn_name[row["svn_name"]] = row["github_name"]

    github_name_by_git_author = {}

    with open("git2github.csv", newline="") as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            github_name_by_git_author[row["git_author"]] = row["github_name"]

    lines = []

    for commit in commits:
        if commit["author_email"].endswith("users.noreply.github.com"):
            github_name = commit["author_email"].split("@")[0]
            if "+" in github_name:
                github_name = github_name.split("+")[1]
            github_name = f"@{github_name}"
        else:
            git_author = f"{commit['author_name']} <{commit['author_email']}>"
            if git_author not in svn_name_by_git_author:
                github_name = github_name_by_git_author[git_author]
                github_name = f"@{github_name}"
            else:
                try:
                    svn_name = svn_name_by_git_author[git_author]
                    github_name = github_name_by_svn_name[svn_name]
                    github_name = f"@{github_name}"
                except KeyError:
                    github_name = git_author

        lines.append(f"- {commit['message']} by {github_name}")

    for line in sorted(lines):
        print(line)

    binder_image_url = "https://camo.githubusercontent.com/581c077bdbc6ca6899c86d0acc6145ae85e9d80e6f805a1071793dbe48917982/68747470733a2f2f6d7962696e6465722e6f72672f62616467655f6c6f676f2e737667"
    binder_url = f"https://mybinder.org/v2/gh/OSGeo/grass/{end_tag}?urlpath=lab%2Ftree%2Fdoc%2Fnotebooks%2Fbasic_example.ipynb"
    print(f"\n[![Binder]({binder_image_url})]({binder_url})")


if __name__ == "__main__":
    main()
