##############################################################################
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Python-driven CLI interface
#
# COPYRIGHT: (C) 2025 Vaclav Petras and the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
##############################################################################

"""
Experimental low-level CLI interface for the main GRASS executable functionality

This is not a stable part of the API. Contact developers before using it.
"""

import argparse
import tempfile
import os
import sys
import subprocess
from pathlib import Path


import grass.script as gs
from grass.app.data import lock_mapset, unlock_mapset, MapsetLockingException
from grass.experimental.tools import Tools


def subcommand_run_tool(args, tool_args: list, help: bool):
    command = [args.tool_name, *tool_args]
    with tempfile.TemporaryDirectory() as tmp_dir_name:
        project_name = "project"
        project_path = Path(tmp_dir_name) / project_name
        gs.create_project(project_path)
        with gs.setup.init(project_path) as session:
            if help:
                result = subprocess.run(command, env=session.env)
                return result.returncode
            tools = Tools(capture_output=False)
            try:
                tools.run_from_list(command)
            except subprocess.CalledProcessError as error:
                return error.returncode


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


def call_g_manual(**kwargs):
    with tempfile.TemporaryDirectory() as tmp_dir_name:
        project_name = "project"
        project_path = Path(tmp_dir_name) / project_name
        gs.create_project(project_path)
        with gs.setup.init(project_path) as session:
            return gs.run_command(
                "g.manual", **kwargs, env=session.env, errors="status"
            )


def subcommand_show_help(args):
    return call_g_manual(entry=args.page)


def subcommand_show_man(args):
    return call_g_manual(entry=args.page, flags="m")


def main(args=None, program=None):
    # Top-level parser
    if program is None:
        program = os.path.basename(sys.argv[0])
        if program == "__main__.py":
            program = f"{Path(sys.executable).name} -m grass.app"
    parser = argparse.ArgumentParser(
        description="Experimental low-level CLI interface to GRASS. Consult developers before using it.",
        prog=program,
    )
    subparsers = parser.add_subparsers(
        title="subcommands", required=True, dest="subcommand"
    )

    # Subcommand parsers

    subparser = subparsers.add_parser("run", help="run a tool")
    subparser.add_argument("tool_name", type=str)
    subparser.set_defaults(func=subcommand_run_tool)

    subparser = subparsers.add_parser("lock", help="lock a mapset")
    subparser.add_argument("mapset_path", type=str)
    subparser.add_argument(
        "--process-id",
        metavar="PID",
        type=int,
        default=1,
        help=_(
            "process ID of the process locking the mapset (a mapset can be "
            "automatically unlocked if there is no process with this PID)"
        ),
    )
    subparser.add_argument(
        "--timeout",
        metavar="TIMEOUT",
        type=float,
        default=30,
        help=_("mapset locking timeout in seconds"),
    )
    subparser.add_argument(
        "-f",
        "--force-remove-lock",
        action="store_true",
        help=_("remove lock if present"),
    )
    subparser.set_defaults(func=subcommand_lock_mapset)

    subparser = subparsers.add_parser("unlock", help="unlock a mapset")
    subparser.add_argument("mapset_path", type=str)
    subparser.set_defaults(func=subcommand_unlock_mapset)

    subparser = subparsers.add_parser(
        "help", help="show HTML documentation for a tool or topic"
    )
    subparser.add_argument("page", type=str)
    subparser.set_defaults(func=subcommand_show_help)

    subparser = subparsers.add_parser(
        "man", help="show man page for a tool or topic using"
    )
    subparser.add_argument("page", type=str)
    subparser.set_defaults(func=subcommand_show_man)

    # Parsing

    if not args:
        args = sys.argv[1:]
    raw_args = args.copy()
    add_back = None
    if len(raw_args) > 2 and raw_args[0] == "run":
        # Getting the --help of tools needs to work around the standard help mechanism
        # of argparse.
        # Maybe a better workaround is to use custom --help, action="help", print_help,
        # and dedicated tool help function complimentary with g.manual subcommand
        # interface.
        if "--help" in raw_args[2:]:
            raw_args.remove("--help")
            add_back = "--help"
        elif "--h" in raw_args[2:]:
            raw_args.remove("--h")
            add_back = "--h"
    parsed_args, other_args = parser.parse_known_args(raw_args)
    if parsed_args.subcommand == "run":
        if add_back:
            other_args.append(add_back)
        return parsed_args.func(parsed_args, other_args, help=bool(add_back))
    return parsed_args.func(parsed_args)
