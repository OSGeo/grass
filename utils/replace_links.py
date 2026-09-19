#!/usr/bin/env python3

import re
import argparse
import difflib
from pathlib import Path


def replace_links_in_markdown(
    filepath, replace_url, replacement, dry_run, verbose, diff
):
    content = Path(filepath).read_text()
    # Match Markdown links/images with multiline labels.
    pattern = re.compile(
        r"(!?\[.*?\])\(\s*" + re.escape(replace_url) + r"([^)]*)?\s*\)", re.DOTALL
    )
    if pattern.search(content):
        new_content = pattern.sub(
            lambda m: f"{m.group(1)}({replacement}{m.group(2) or ''})", content
        )
        if not dry_run:
            Path(filepath).write_text(new_content)
        if (verbose or dry_run) and not diff:
            text = "Would update" if dry_run else "Updated"
            print(f"{text}: {filepath}")
        if diff:
            difference = difflib.unified_diff(
                content.splitlines(),
                new_content.splitlines(),
                fromfile=f"a/{filepath}",
                tofile=f"b/{filepath}",
                lineterm="",
            )
            print("\n".join(difference))


def ensure_trailing_slash(url):
    return url.rstrip("/") + "/" if url else ""


def replace_grass_links(args):
    # Normalize URL format.
    replace_url = ensure_trailing_slash(args.replace_url)
    new_base_url = ensure_trailing_slash(args.new_base_url)

    for subdir in args.target_dirs:
        dir_path = Path(args.root_dir) / subdir
        replacement = new_base_url if subdir == "" else f"{new_base_url}.."
        # This assumes that the directory exists.
        for file_path in Path.iterdir(dir_path):
            if file_path.is_file() and file_path.suffix == args.file_ext:
                replace_links_in_markdown(
                    file_path,
                    replace_url,
                    replacement,
                    dry_run=args.dry_run,
                    diff=args.diff,
                    verbose=args.verbose,
                )


def main():
    parser = argparse.ArgumentParser(
        description="Replace GRASS manual URLs in Markdown links with relative or custom paths.",
        epilog="""
Example:
  python replace_grass_links.py \\
    "https://example.org/manuals" \\
    ./docs \\
    --target-dirs "" addons libpython \\
    --new-base-url "" \\
    --file-ext .md \\
    --verbose
""",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "replace_url",
        default="",
        help="URL string to replace (default: %(default)s)",
    )
    parser.add_argument(
        "root_dir",
        help="Path to the root directory containing Markdown files and subdirectories.",
    )
    parser.add_argument(
        "--new-base-url",
        default="",
        help="Replacement string for the base URL (default: empty for relative links)",
    )
    parser.add_argument(
        "--target-dirs",
        nargs="+",
        default=[""],
        help="Subdirectories to process relative to root (default: %(default)s)",
    )
    parser.add_argument(
        "--file-ext",
        default=".md",
        help="File extension to process (default: %(default)s)",
    )
    parser.add_argument(
        "--dry-run", action="store_true", help="Preview changes without modifying files"
    )
    parser.add_argument(
        "--diff",
        action="store_true",
        help="Print difference between new and old content",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print every file that gets (or would be) updated",
    )

    args = parser.parse_args()
    replace_grass_links(args)


if __name__ == "__main__":
    main()
