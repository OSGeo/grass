# common functions used by mkmarkdown.py and mkhtml.py

import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import urllib.parse as urlparse
from datetime import datetime
from http import HTTPStatus
from pathlib import Path
from urllib import request as urlrequest
from urllib.error import HTTPError, URLError

try:
    import grass.script as gs
except ImportError:
    # During compilation GRASS GIS
    gs = None

from generate_last_commit_file import COMMIT_DATE_FORMAT

HEADERS = {
    "User-Agent": "Mozilla/5.0",
}
HTTP_STATUS_CODES = list(HTTPStatus)

top_dir = os.path.abspath(os.getenv("MODULE_TOPDIR"))


def read_file(name):
    try:
        return Path(name).read_text()
    except OSError:
        return ""


def set_proxy():
    """Set proxy"""
    proxy = os.getenv("GRASS_PROXY")
    if proxy:
        proxies = {}
        for ptype, purl in (p.split("=") for p in proxy.split(",")):
            proxies[ptype] = purl
        urlrequest.install_opener(
            urlrequest.build_opener(urlrequest.ProxyHandler(proxies))
        )


def get_version_branch(major_version, addons_git_repo_url):
    """Check if version branch for the current GRASS version exists,
    if not, take branch for the previous version
    For the official repo we assume that at least one version branch is present

    :param major_version int: GRASS GIS major version
    :param addons_git_repo_url str: Addons Git repository URL

    :return version_branch str: version branch
    """
    version_branch = f"grass{major_version}"
    if gs:
        branch = gs.Popen(
            [
                "git",
                "ls-remote",
                "--heads",
                addons_git_repo_url,
                f"refs/heads/{version_branch}",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        branch, stderr = branch.communicate()
        if stderr:
            gs.fatal(
                _(
                    "Failed to get branch from the Git repository"
                    " <{repo_path}>.\n{error}"
                ).format(
                    repo_path=addons_git_repo_url,
                    error=gs.decode(stderr),
                )
            )
        if version_branch not in gs.decode(branch):
            version_branch = "grass{}".format(int(major_version) - 1)
    return version_branch


def has_src_code_git(src_dir):
    """Has core module or addon source code Git

    :param str src_dir: core module or addon root directory

    :return subprocess.CompletedProcess or None: subprocess.CompletedProcess
                                                 if core module or addon
                                                 source code has Git
    """
    actual_dir = Path.cwd()
    os.chdir(src_dir)
    try:
        process_result = subprocess.run(
            [
                "git",
                "log",
                "-1",
                f"--format=%H,{COMMIT_DATE_FORMAT}",
                src_dir,
            ],
            capture_output=True,
        )  # --format=%H,COMMIT_DATE_FORMAT commit hash,author date
        os.chdir(actual_dir)
        return process_result if process_result.returncode == 0 else None
    except FileNotFoundError:
        os.chdir(actual_dir)
        return None


def get_last_git_commit(src_dir, top_dir, pgm, addon_path, major_version):
    """Get last module/addon git commit
    :param str src_dir: module/addon source dir
    :param str top_dir: top source dir
    :param str pgm: program name
    :param str addon_path: addon path
    :param str major_version: major GRASS version

    :return dict git_log: dict with key commit and date, if not
                          possible download commit from GitHub REST API
                          server values of keys have "unknown" string
    """
    process_result = has_src_code_git(src_dir=src_dir)
    if process_result:
        return parse_git_commit(
            commit=process_result.stdout.decode(),
            src_dir=src_dir,
        )
    if gs:
        # Addons installation
        return get_git_commit_from_rest_api_for_addon_repo(
            addon_path=addon_path, src_dir=src_dir, pgm=pgm, major_version=major_version
        )
    # During GRASS GIS compilation from source code without Git
    return get_git_commit_from_file(src_dir=src_dir, pgm=pgm)


def parse_git_commit(
    commit,
    src_dir,
    git_log=None,
):
    """Parse Git commit

    :param str commit: commit message
    :param str src_dir: addon source dir
    :param dict git_log: dict which store last commit and commnit
                         date

    :return dict git_log: dict which store last commit and commnit date
    """
    if not git_log:
        git_log = get_default_git_log(src_dir=src_dir)
    if commit:
        git_log["commit"], commit_date = commit.strip().split(",")
        git_log["date"] = format_git_commit_date_from_local_git(
            commit_datetime=commit_date,
        )
    return git_log


def get_default_git_log(src_dir, datetime_format="%A %b %d %H:%M:%S %Y"):
    """Get default Git commit and commit date, when getting commit from
    local Git, local JSON file and remote GitHub REST API server wasn't
    successful.

    :param str src_dir: addon source dir
    :param str datetime_format: output commit datetime format
                                e.g. Sunday Jan 16 23:09:35 2022

    :return dict: dict which store last commit and commnit date
    """
    return {
        "commit": "unknown",
        "date": datetime.fromtimestamp(os.path.getmtime(src_dir)).strftime(
            datetime_format
        ),
    }


def format_git_commit_date_from_local_git(
    commit_datetime, datetime_format="%A %b %d %H:%M:%S %Y"
):
    """Format datetime from local Git or JSON file

    :param str commit_datetime: commit datetime
    :param str datetime_format: output commit datetime format
                                e.g. Sunday Jan 16 23:09:35 2022

    :return str: output formatted commit datetime
    """
    try:
        date = datetime.fromisoformat(
            commit_datetime,
        )
    except ValueError:
        if commit_datetime.endswith("Z"):
            # Python 3.10 and older does not support Z in time, while recent versions
            # of Git (2.45.1) use it. Try to help the parsing if Z is in the string.
            date = datetime.fromisoformat(commit_datetime[:-1] + "+00:00")
        else:
            raise
    return date.strftime(datetime_format)


def get_git_commit_from_rest_api_for_addon_repo(
    addon_path,
    src_dir,
    pgm,
    major_version,
    git_log=None,
):
    """Get Git commit from remote GitHub REST API for addon repository

    :param str addon_path: addon path
    :param str src_dir: addon source dir
    :param str pgm: program name
    :param major_version int: GRASS GIS major version
    :param dict git_log: dict which store last commit and commnit date

    :return dict git_log: dict which store last commit and commnit date
    """
    # Accessed date time if getting commit from GitHub REST API wasn't successful
    if not git_log:
        git_log = get_default_git_log(src_dir=src_dir)
    if addon_path is not None:
        grass_addons_url = (
            "https://api.github.com/repos/osgeo/grass-addons/commits?"
            "path={path}&page=1&per_page=1&sha=grass{major}".format(
                path=addon_path,
                major=major_version,
            )
        )  # sha=git_branch_name

        response = download_git_commit(
            url=grass_addons_url,
            pgm=pgm,
            response_format="application/json",
        )
        if response:
            commit = json.loads(response.read())
            if commit:
                git_log["commit"] = commit[0]["sha"]
                git_log["date"] = format_git_commit_date_from_rest_api(
                    commit_datetime=commit[0]["commit"]["author"]["date"],
                )
    return git_log


def get_git_commit_from_file(
    src_dir,
    pgm,
    git_log=None,
):
    """Get Git commit from JSON file

    :param str src_dir: addon source dir
    :param str pgm: program name
    :param dict git_log: dict which store last commit and commnit date

    :return dict git_log: dict which store last commit and commnit date
    """
    # Accessed date time if getting commit from JSON file wasn't successful
    if not git_log:
        git_log = get_default_git_log(src_dir=src_dir)
    json_file_path = os.path.join(
        top_dir,
        "core_modules_with_last_commit.json",
    )
    if os.path.exists(json_file_path):
        with open(json_file_path) as f:
            core_modules_with_last_commit = json.load(f)
        if pgm in core_modules_with_last_commit:
            core_module = core_modules_with_last_commit[pgm]
            git_log["commit"] = core_module["commit"]
            git_log["date"] = format_git_commit_date_from_local_git(
                commit_datetime=core_module["date"],
            )
    return git_log


def download_git_commit(url, pgm, response_format, *args, **kwargs):
    """Download module/addon last commit from GitHub API

    :param str url: url address
    :param str pgm: program name
    :param str response_format: content type

    :return urllib.request.urlopen or None response: response object or
                                                     None
    """
    try:
        response = urlopen(url, *args, **kwargs)
        if response.code != 200:
            index = HTTP_STATUS_CODES.index(response.code)
            desc = HTTP_STATUS_CODES[index].description
            gs.fatal(
                _(
                    "Download commit from <{url}>, return status code {code}, {desc}"
                ).format(
                    url=url,
                    code=response.code,
                    desc=desc,
                ),
            )
        if response_format not in response.getheader("Content-Type"):
            gs.fatal(
                _(
                    "Wrong downloaded commit file format. "
                    "Check url <{url}>. Allowed file format is "
                    "{response_format}."
                ).format(
                    url=url,
                    response_format=response_format,
                ),
            )
        return response
    except HTTPError as err:
        gs.warning(
            _(
                "The download of the commit from the GitHub API "
                "server wasn't successful, <{}>. Commit and commit "
                "date will not be included in the <{}> addon html manual "
                "page."
            ).format(err.msg, pgm),
        )
    except URLError:
        gs.warning(
            _(
                "Download file from <{url}>, failed. Check internet "
                "connection. Commit and commit date will not be included "
                "in the <{pgm}> addon manual page."
            ).format(url=url, pgm=pgm),
        )


def format_git_commit_date_from_rest_api(
    commit_datetime, datetime_format="%A %b %d %H:%M:%S %Y"
):
    """Format datetime from remote GitHub REST API

    :param str commit_datetime: commit datetime
    :param str datetime_format: output commit datetime format
                                e.g. Sunday Jan 16 23:09:35 2022

    :return str: output formatted commit datetime
    """
    return datetime.strptime(
        commit_datetime,
        "%Y-%m-%dT%H:%M:%SZ",  # ISO 8601 YYYY-MM-DDTHH:MM:SSZ
    ).strftime(datetime_format)


def urlopen(url, *args, **kwargs):
    """Wrapper around urlopen. Same function as 'urlopen', but with the
    ability to define headers.
    """
    request = urlrequest.Request(url, headers=HEADERS)
    return urlrequest.urlopen(request, *args, **kwargs)


def get_addon_path(base_url, pgm, major_version):
    """Check if pgm is in the addons list and get addon path

    Make or update list of the official addons source
    code paths g.extension prefix parameter plus /grass-addons directory
    using Git repository

    :param str base_url: base URL
    :param str pgm: program name
    :param str major_version: GRASS major version

    :return str|None: pgm path if pgm is addon else None
    """
    addons_base_dir = os.getenv("GRASS_ADDON_BASE")
    if not addons_base_dir or not major_version:
        return None
    grass_addons_dir = Path(addons_base_dir) / "grass-addons"
    if gs:
        call = gs.call
        popen = gs.Popen
        fatal = gs.fatal
    else:
        call = subprocess.call
        popen = subprocess.Popen
        fatal = sys.stderr.write
    addons_branch = get_version_branch(
        major_version=major_version,
        addons_git_repo_url=urlparse.urljoin(base_url, "grass-addons/"),
    )
    if not Path(addons_base_dir).exists():
        Path(addons_base_dir).mkdir(parents=True, exist_ok=True)
    if not grass_addons_dir.exists():
        try:
            with tempfile.TemporaryDirectory(dir=addons_base_dir) as tmpdir:
                tmp_clone_path = Path(tmpdir) / "grass-addons"
                call(
                    [
                        "git",
                        "clone",
                        "-q",
                        "--no-checkout",
                        f"--branch={addons_branch}",
                        "--filter=blob:none",
                        urlparse.urljoin(base_url, "grass-addons/"),
                        str(tmp_clone_path),
                    ]
                )
                shutil.move(tmp_clone_path, grass_addons_dir)
        except (shutil.Error, OSError):
            if not grass_addons_dir.exists():
                raise
    addons_file_list = popen(
        ["git", "ls-tree", "--name-only", "-r", addons_branch],
        cwd=grass_addons_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    addons_file_list, stderr = addons_file_list.communicate()
    if stderr:
        message = (
            "Failed to get addons files list from the"
            " Git repository <{repo_path}>.\n{error}"
        )
        if gs:
            fatal(
                _(
                    message,
                ).format(
                    repo_path=grass_addons_dir,
                    error=gs.decode(stderr),
                )
            )
        else:
            message += "\n"
            fatal(
                message.format(
                    repo_path=grass_addons_dir,
                    error=stderr.decode(),
                )
            )
    addon_paths = re.findall(
        rf".*{pgm}*.",
        gs.decode(addons_file_list) if gs else addons_file_list.decode(),
    )
    for addon_path in addon_paths:
        if pgm == Path(addon_path).name:
            return addon_path
