"""Experimental low-level CLI interface for the main GRASS executable functionality

(C) 2025 by Vaclav Petras and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

.. sectionauthor:: Vaclav Petras <wenzeslaus gmail com>

This is not a stable part of the API. Use at your own risk.
"""

import argparse
import os
import sys
from pathlib import Path

import grass.script as gs
from grass.app.data import lock_mapset, unlock_mapset, MapsetLockingException


def subcommand_lock_mapset(args):
    gs.setup.setup_runtime_env()
    try:
        lock_mapset(
            args.mapset_path,
            force_lock_removal=args.force_remove_lock,
            timeout=args.timeout,
            message_callback=print,
            process_id=args.process_id,
        )
    except MapsetLockingException as e:
        print(str(e), file=sys.stderr)


def subcommand_unlock_mapset(args):
    unlock_mapset(args.mapset_path)


def main():
    # Top-level parser
    program = os.path.basename(sys.argv[0])
    if program == "__main__.py":
        program = f"{Path(sys.executable).name} -m grass.app"
    parser = argparse.ArgumentParser(
        description="Experimental low-level CLI interface to GRASS. Consult developers before using it.",
        prog=program,
    )
    subparsers = parser.add_subparsers(title="subcommands", required=True)

    # Subcommand parsers

    parser_foo = subparsers.add_parser("lock", help="lock a mapset")
    parser_foo.add_argument("mapset_path", type=str)
    parser_foo.add_argument(
        "--process-id",
        metavar="PID",
        type=int,
        default=1,
        help=_(
            "process ID of the process locking the mapset (a mapset can be "
            "automatically unlocked if there is no process with this PID)"
        ),
    )
    parser_foo.add_argument(
        "--timeout",
        metavar="TIMEOUT",
        type=float,
        default=30,
        help=_("mapset locking timeout in seconds"),
    )
    parser_foo.add_argument(
        "-f",
        "--force-remove-lock",
        action="store_true",
        help=_("remove lock if present"),
    )
    parser_foo.set_defaults(func=subcommand_lock_mapset)

    parser_bar = subparsers.add_parser("unlock", help="unlock a mapset")
    parser_bar.add_argument("mapset_path", type=str)
    parser_bar.set_defaults(func=subcommand_unlock_mapset)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
