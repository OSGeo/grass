# MODULE:    grass.utils
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Collection of various helper general (non-GRASS) utilities
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Download and extract various archives"""

import os
import re
import shutil
import tarfile
import tempfile
import zipfile
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urlparse
from urllib.request import urlretrieve


response_content_type_header_pattern = re.compile(r"application/(zip|octet-stream)")
response_content_disposition_header_pattern = re.compile(
    r"attachment; filename=.*.zip$"
)

# Schemes urlretrieve can fetch, except data. The file scheme allows an archive
# on the local disk.
supported_url_schemes = ("http", "https", "ftp", "file")


def debug(*args, **kwargs):
    """Print a debug message (to be used in this module only)

    Using the stanard grass.script debug function is nice, but it may create a circular
    dependency if this is used from grass.script, so this is a wrapper which lazy
    imports the standard function.
    """
    # Lazy import to avoiding potential circular dependency.
    import grass.script as gs  # pylint: disable=import-outside-toplevel

    gs.debug(*args, **kwargs)


class DownloadError(Exception):
    """Error happened during download or when processing the file"""


# modified copy from g.extension
# TODO: Possibly migrate to shutil.unpack_archive.
def extract_tar(name, directory, tmpdir):
    """Extract a TAR or a similar file into a directory"""
    debug(
        f"extract_tar(name={name}, directory={directory}, tmpdir={tmpdir})",
        3,
    )
    try:
        tar = tarfile.open(name)
        extract_dir = os.path.join(tmpdir, "extract_dir")
        Path(extract_dir).mkdir()

        # The 'data' extraction filter was added in Python 3.12 and backported
        # to 3.11.4 (PEP 706). Refuse to extract without it rather
        # than extracting unsafely.
        if not hasattr(tarfile, "data_filter"):
            raise DownloadError(
                _("Extracting may be unsafe; upgrade Python to 3.11.4 or newer")
            )
        tar.extractall(path=extract_dir, filter="data")

        files = os.listdir(extract_dir)
        _move_extracted_files(
            extract_dir=extract_dir, target_dir=directory, files=files
        )
    except tarfile.TarError as error:
        raise DownloadError(
            _("Archive file is unreadable: {0}").format(error)
        ) from error
    except EOFError as error:
        raise DownloadError(
            _("Archive file is incomplete: {0}").format(error)
        ) from error


extract_tar.supported_formats = ["tar.gz", "gz", "bz2", "tar", "gzip", "targz", "xz"]


# modified copy from g.extension
# TODO: Possibly migrate to shutil.unpack_archive.
def extract_zip(name, directory, tmpdir):
    """Extract a ZIP file into a directory"""
    debug(
        f"extract_zip(name={name}, directory={directory}, tmpdir={tmpdir})",
        3,
    )
    try:
        zip_file = zipfile.ZipFile(name, mode="r")
        file_list = zip_file.namelist()
        # we suppose we can write to parent of the given dir
        # (supposing a tmp dir)
        extract_dir = os.path.join(tmpdir, "extract_dir")
        Path(extract_dir).mkdir()
        for subfile in file_list:
            # this should be safe in Python 2.7.4
            zip_file.extract(subfile, extract_dir)
        files = os.listdir(extract_dir)
        _move_extracted_files(
            extract_dir=extract_dir, target_dir=directory, files=files
        )
    except zipfile.BadZipfile as error:
        raise DownloadError(_("ZIP file is unreadable: {0}").format(error))


# modified copy from g.extension
def _move_extracted_files(extract_dir, target_dir, files):
    """Fix state of extracted file by moving them to different directory

    When extracting, it is not clear what will be the root directory
    or if there will be one at all. So this function moves the files to
    a different directory in the way that if there was one directory extracted,
    the contained files are moved.
    """
    debug("_move_extracted_files({})".format(locals()))
    if len(files) == 1:
        actual_path = os.path.join(extract_dir, files[0])
        if Path(actual_path).is_dir():
            shutil.copytree(actual_path, target_dir)
        else:
            shutil.copy(actual_path, target_dir)
    else:
        Path(target_dir).mkdir(exist_ok=True)
        for file_name in files:
            actual_file = os.path.join(extract_dir, file_name)
            if Path(actual_file).is_dir():
                # Choice of copy tree function:
                # shutil.copytree() fails when subdirectory exists.
                # However, distutils.copy_tree() may fail to create directories before
                # copying files into them when copying to a recently deleted directory.
                shutil.copytree(actual_file, os.path.join(target_dir, file_name))
            else:
                shutil.copy(actual_file, os.path.join(target_dir, file_name))


def _download_file(source, destination, reporthook=None):
    """Download a file from URL to a local file

    Only URLs with one of the supported schemes are downloaded. A file on the
    local disk needs to be specified with the file scheme, i.e., as file://<path>.

    :param source: URL to download from
    :param destination: local file name to download to
    :param reporthook: function called with the download progress

    :return: tuple with the local file name and the response headers
    """
    if urlparse(source).scheme not in supported_url_schemes:
        raise DownloadError(
            _(
                "Unsupported URL <{url}>. Supported are only URLs with the "
                "following schemes: {schemes}."
            ).format(url=source, schemes=", ".join(supported_url_schemes))
        )
    try:
        # The scheme of the URL is limited to the supported ones above.
        return urlretrieve(source, destination, reporthook)  # nosec B310
    except HTTPError as error:
        raise DownloadError(
            _("Download file from <{url}>, return status code {code}, {desc}").format(
                url=source, code=error.code, desc=error.reason
            )
        ) from error
    except URLError as error:
        raise DownloadError(
            _("Download file from <{url}>, failed. Check internet connection.").format(
                url=source
            )
        ) from error


# modified copy from g.extension
# TODO: remove the hardcoded location/extension, use general name
def download_and_extract(source, reporthook=None):
    """Download a file (archive) from URL and extract it

    Call urllib.request.urlcleanup() to clean up after urlretrieve if you terminate
    this function from another thread.
    """
    source_path = Path(urlparse(source).path)
    tmpdir = tempfile.mkdtemp()
    debug("Tmpdir: {}".format(tmpdir))
    directory = Path(tmpdir) / "extracted"
    if source_path.suffix and source_path.suffix == ".zip":
        archive_name = os.path.join(tmpdir, "archive.zip")
        filename, headers = _download_file(source, archive_name, reporthook)

        # Check that a remote download looks like a ZIP archive by its response
        # headers to catch a server returning an error page instead of the file.
        # A local file (the file scheme) has no such response: urlretrieve
        # synthesizes the content type from the file name, which is unreliable
        # (the Windows registry maps .zip to application/x-zip-compressed), so
        # the check is skipped for it. A local file which is not a ZIP is caught
        # by extract_zip.
        if (
            urlparse(source).scheme != "file"
            and not re.search(
                response_content_type_header_pattern, headers.get("content-type", "")
            )
            and not re.search(
                response_content_disposition_header_pattern,
                headers.get("content-disposition", ""),
            )
        ):
            raise DownloadError(
                _(
                    "Download of <{url}> failed or file <{name}> is not a ZIP file"
                ).format(url=source, name=filename)
            )
        extract_zip(name=archive_name, directory=directory, tmpdir=tmpdir)
    elif source_path.suffix and source_path.suffix[1:] in extract_tar.supported_formats:
        ext = "".join(source_path.suffixes)
        archive_name = os.path.join(tmpdir, "archive" + ext)
        _download_file(source, archive_name, reporthook)
        extract_tar(name=archive_name, directory=directory, tmpdir=tmpdir)
    else:
        # probably programmer error
        raise DownloadError(_("Unknown format '{}'.").format(source))
    return directory


def name_from_url(url):
    """Extract name from URL"""
    name = os.path.basename(urlparse(url).path)
    name = os.path.splitext(name)[0]
    if name.endswith(".tar"):
        # Special treatment of .tar.gz extension.
        return os.path.splitext(name)[0]
    return name
