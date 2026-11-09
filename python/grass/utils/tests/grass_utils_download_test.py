"""Tests of grass.utils.download"""

import io
import tarfile
import zipfile

import pytest

from grass.utils.download import DownloadError, download_and_extract


@pytest.fixture
def zip_archive(tmp_path):
    """ZIP archive with a project directory"""
    archive = tmp_path / "project.zip"
    with zipfile.ZipFile(archive, "w") as archive_file:
        archive_file.writestr("project/PERMANENT/DEFAULT_WIND", "proj: 3\n")
    return archive


@pytest.fixture
def tar_archive(tmp_path):
    """Compressed TAR archive with a project directory"""
    archive = tmp_path / "project.tar.gz"
    content = b"proj: 3\n"
    with tarfile.open(archive, "w:gz") as archive_file:
        info = tarfile.TarInfo("project/PERMANENT/DEFAULT_WIND")
        info.size = len(content)
        archive_file.addfile(info, io.BytesIO(content))
    return archive


@pytest.mark.parametrize("archive", ["zip_archive", "tar_archive"])
def test_extract_from_file_url(request, archive):
    """Archive on the local disk is extracted when given with the file scheme"""
    directory = download_and_extract(request.getfixturevalue(archive).as_uri())

    assert (directory / "PERMANENT" / "DEFAULT_WIND").is_file()


def test_extract_from_file_url_not_a_zip(tmp_path):
    """A local file with a .zip name which is not a ZIP is reported as an error

    The content type of a file URL is not checked (it is unreliable), so an
    invalid ZIP has to be caught during extraction.
    """
    archive = tmp_path / "project.zip"
    archive.write_text("This is not a ZIP file.\n")

    with pytest.raises(DownloadError, match="ZIP file is unreadable"):
        download_and_extract(archive.as_uri())


@pytest.mark.parametrize(
    "source",
    [
        "/home/user/project.zip",
        "C:\\data\\project.zip",
        "custom://example.org/project.zip",
    ],
)
def test_unsupported_url_scheme(source):
    """URL with an unsupported scheme is rejected

    A path without a scheme is not a URL, so it is rejected as well.
    """
    with pytest.raises(DownloadError, match="Unsupported URL"):
        download_and_extract(source)
