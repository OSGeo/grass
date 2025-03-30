#!/usr/bin/env python3

"""
Created on March 11, 2025

@author: Corey White
"""

import os
import argparse
from xml.dom import minidom  # noqa: S408
import urllib.parse
from pathlib import Path


def check_url_version(url: str, version: str) -> str:
    """
    Check if the version in the URL matches the provided version. If not, update it."
    """
    # Parse the URL
    parsed = urllib.parse.urlparse(url)
    path_parts = parsed.path.split("/")

    # Check if there is at least one segment after the initial empty string.
    if len(path_parts) > 1 and path_parts[1]:
        # If the version in the URL doesn't match the provided version, update it.
        if path_parts[1] != version:
            path_parts[1] = version
            # Reassemble the path (the leading '/' will be preserved by join since the first element is empty)
            new_path = "/".join(path_parts)
            # Construct the updated URL
            return urllib.parse.urlunparse(parsed._replace(path=new_path))
    # If no update is needed, return the original URL.
    return url


def import_nodes(
    source_root: minidom.Document, merged_doc: minidom.Document, version: str
) -> minidom.Document:
    """
    Import the <url> elements from the source document into the merged document.
    """
    # Get the <urlset> element from the merged document
    urlset_elements = merged_doc.getElementsByTagName("urlset")
    if not urlset_elements:
        error_message = "Merged document does not contain a <urlset> element."
        raise ValueError(error_message)
    urlset_element = urlset_elements[0]

    # Iterate over the <url> elements in the source document
    for element in source_root.getElementsByTagName("url"):
        # Update the <loc> element to match the provided version
        element_loc_value = (
            element.getElementsByTagName("loc")[0].childNodes[0].data.strip()
        )
        updated_url = check_url_version(element_loc_value, version)
        element.getElementsByTagName("loc")[0].childNodes[0].data = updated_url

        # Clone the <url> element and append it to the <urlset> element in the merged document
        cloned_element = element.cloneNode(deep=True)
        urlset_element.appendChild(cloned_element)

    return merged_doc


def main():
    parser = argparse.ArgumentParser(
        description="Merge XML sitemaps for GRASS GIS manual pages (MKDocs) and libpython (Sphinx)"
    )
    parser.add_argument(
        "--mkdocs-sitemap",
        type=str,
        required=True,
        help="The path to the MKDocs sitemap.xml",
    )
    parser.add_argument(
        "--sphinx-sitemap",
        type=str,
        required=True,
        help="The path to the libpython sitemap.xml",
    )
    parser.add_argument(
        "--version",
        type=str,
        help="The version manual, (default: %(default)s)",
        default="grass-stable",
    )
    parser.add_argument(
        "--output",
        type=str,
        help="The path to the output file (default: %(default)s)",
        default="sitemap_merged.xml",
    )
    parser.add_argument("-o", "--overwrite", dest="overwrite", action="store_true")

    # Check arguments
    args = parser.parse_args()
    mkdocs_sitemap = args.mkdocs_sitemap
    if not os.path.exists(mkdocs_sitemap):
        error_message = "MKDocs sitemap.xml does not exist"
        raise FileNotFoundError(error_message)

    sphinx_sitemap = args.sphinx_sitemap
    if not sphinx_sitemap:
        error_message = (
            "Sphinx sitemap is not provided. Only MKDocs sitemap will be merged"
        )
        raise FileNotFoundError(error_message)

    version = args.version

    output = Path(args.output)
    if output.exists() and not args.overwrite:
        print(
            "{} already exists. If you want overwrite please use '-o' parameter".format(
                output
            )
        )
        return False

    print("Merging sitemaps...")
    print(f"MKDocs sitemap: {mkdocs_sitemap}")
    print(f"Sphinx sitemap: {sphinx_sitemap}")
    print(f"Output: {output}")

    # Create the merged document
    merged_doc = minidom.Document()
    merged_doc.encoding = "UTF-8"
    merged_root = merged_doc.createElement("urlset")
    merged_root.setAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
    merged_root.setAttribute(
        "xsi:schemaLocation",
        (
            "http://www.sitemaps.org/schemas/sitemap/0.9 http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd"
        ),
    )
    merged_root.setAttribute("xmlns", "https://www.sitemaps.org/schemas/sitemap/0.9")
    merged_doc.appendChild(merged_root)

    # Import the <url> elements from the source documents into the merged document
    with (
        minidom.parse(mkdocs_sitemap) as mkdocs_sitemap,  # noqa: S318
        minidom.parse(sphinx_sitemap) as sphinx_sitemap,  # noqa: S318
    ):
        merged_doc = import_nodes(mkdocs_sitemap, merged_doc, version)
        merged_doc = import_nodes(sphinx_sitemap, merged_doc, version)

    # Write the merged document to the output file
    xml_str = merged_doc.toprettyxml(indent="   ", encoding="utf-8").decode("utf-8")
    clean_xml = "\n".join([line for line in xml_str.splitlines() if line.strip()])
    output.write_text(clean_xml, encoding="utf-8")

    print("Sitemaps merged successfully.")


if __name__ == "__main__":
    main()
