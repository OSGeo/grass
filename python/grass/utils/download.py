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


reponse_content_type_header_pattern = re.compile(r"application/(zip|octet-stream)")
reponse_content_disposition_header_pattern = re.compile(r"attachment; filename=.*.zip$")


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
        os.mkdir(extract_dir)

        # Extraction filters were added in Python 3.12,
        # and backported to 3.8.17, 3.9.17, 3.10.12, and 3.11.4
        # See
        # https://docs.python.org/3.12/library/tarfile.html#tarfile-extraction-filter
        # and https://peps.python.org/pep-0706/
        # In Python 3.12, using `filter=None` triggers a DepreciationWarning,
        # and in Python 3.14, `filter='data'` will be the default
        if hasattr(tarfile, "data_filter"):
            tar.extractall(path=extract_dir, filter="data")
        else:
            # Remove this when no longer needed
            debug(_("Extracting may be unsafe; consider updating Python"))
            tar.extractall(path=extract_dir)

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
        os.mkdir(extract_dir)
        for subfile in file_list:
            # this should be safe in Python 2.7.4
            zip_file.extract(subfile, extract_dir)
        files = os.listdir(extract_dir)
        _move_extracted_files(
            extract_dir=extract_dir, target_dir=directory, files=files
        )
    except zipfile.BadZipfile as error:
        raise DownloadError(_("ZIP file is unreadable: {0}").format(error)) from error


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
        if os.path.isdir(actual_path):
            shutil.copytree(actual_path, target_dir)
        else:
            shutil.copy(actual_path, target_dir)
    else:
        if not os.path.exists(target_dir):
            os.mkdir(target_dir)
        for file_name in files:
            actual_file = os.path.join(extract_dir, file_name)
            if os.path.isdir(actual_file):
                # Choice of copy tree function:
                # shutil.copytree() fails when subdirectory exists.
                # However, distutils.copy_tree() may fail to create directories before
                # copying files into them when copying to a recently deleted directory.
                shutil.copytree(actual_file, os.path.join(target_dir, file_name))
            else:
                shutil.copy(actual_file, os.path.join(target_dir, file_name))


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
    http_error_message = _("Download file from <{url}>, return status code {code}, ")
    url_error_message = _(
        "Download file from <{url}>, failed. Check internet connection."
    )
    if source_path.suffix and source_path.suffix == ".zip":
        archive_name = os.path.join(tmpdir, "archive.zip")
        try:
            filename, headers = urlretrieve(source, archive_name, reporthook)
        except HTTPError as err:
            raise DownloadError(
                http_error_message.format(
                    url=source,
                    code=err,
                ),
            ) from err
        except URLError as e:
            raise DownloadError(url_error_message.format(url=source)) from e

        if not re.search(
            reponse_content_type_header_pattern, headers.get("content-type", "")
        ) and not re.search(
            reponse_content_disposition_header_pattern,
            headers.get("content-disposition", ""),
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
        try:
            urlretrieve(source, archive_name, reporthook)
        except HTTPError as err:
            raise DownloadError(
                http_error_message.format(
                    url=source,
                    code=err,
                ),
            ) from err
        except URLError as e:
            raise DownloadError(url_error_message.format(url=source)) from e
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
