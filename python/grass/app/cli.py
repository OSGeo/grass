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
from grass.tools import Tools

# Special flags supported besides help and --json which does not need special handling:
SPECIAL_FLAGS = [
    "--interface-description",
    "--md-description",
    "--wps-process-description",
    "--script",
]
# To make this list shorter, we don't support outdated special flags:
# --help-text --html-description --rst-description


def subcommand_run_tool(args, tool_args: list, print_help: bool):
    command = [args.tool, *tool_args]
    with tempfile.TemporaryDirectory() as tmp_dir_name:
        project_name = "project"
        project_path = Path(tmp_dir_name) / project_name
        gs.create_project(project_path)
        with gs.setup.init(project_path) as session:
            tools = Tools(session=session, capture_output=False)
            try:
                # From here, we return the subprocess return code regardless of its
                # value. Error states are handled through exceptions.
                if print_help:
                    # We consumed the help flag, so we need to add it explicitly.
                    return tools.call_cmd([*command, "--help"]).returncode
                if any(item in command for item in SPECIAL_FLAGS):
                    # This is here basically because of how --json behaves,
                    # two JSON flags are accepted, but --json currently overridden by
                    # other special flags, so later use of --json in tools will fail
                    # with the other flags active.
                    return tools.call_cmd(command).returncode
                return tools.run_cmd(command).returncode
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
        title="subcommands", dest="subcommand", required=True
    )

    # Subcommand parsers

    run_subparser = subparsers.add_parser(
        "run",
        help="run a tool",
        add_help=False,
        epilog="Tool name is followed by its parameters.",
    )
    run_subparser.add_argument("tool", type=str, nargs="?", help="name of a tool")
    run_subparser.add_argument("--help", action="store_true")
    run_subparser.set_defaults(func=subcommand_run_tool)

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
    parsed_args, other_args = parser.parse_known_args(args)
    # Standard help already exited, but we need to handle tools separately.
    if parsed_args.subcommand == "run":
        if parsed_args.tool is None and parsed_args.help:
            run_subparser.print_help()
            return 0
        if parsed_args.tool is None:
            run_subparser.print_usage()
            # argparse gives 2 without parameters, so we behave the same.
            return 2
        return parsed_args.func(parsed_args, other_args, print_help=parsed_args.help)
    return parsed_args.func(parsed_args)
