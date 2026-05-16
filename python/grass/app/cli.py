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
from contextlib import ExitStack
from pathlib import Path

import grass.script as gs
from grass.app.data import lock_mapset, unlock_mapset, MapsetLockingException
from grass.grassdb.create import create_mapset
from grass.exceptions import ScriptError
from grass.tools import Tools


# --- Alias Mapping (PoC) ---
COMMAND_MAP = {
    "list": "g.list",
    "slope": "r.slope.aspect",
}


def resolve_tool_name(tool_name: str) -> str:
    """Resolve user-friendly tool aliases to actual GRASS module names."""
    return COMMAND_MAP.get(tool_name, tool_name)


# Special flags supported besides help and --json which does not need special handling:
SPECIAL_FLAGS = [
    "--interface-description",
    "--md-description",
    "--wps-process-description",
    "--script",
]


def subcommand_run_tool(args, tool_args: list, print_help: bool) -> int:
    resolved_tool = resolve_tool_name(args.tool)
    command = [resolved_tool, *tool_args]

    with ExitStack() as stack:
        if args.project:
            project_path = Path(args.project)
        else:
            tmp_dir_name = stack.enter_context(tempfile.TemporaryDirectory())
            project_name = "project"
            project_path = Path(tmp_dir_name) / project_name
            gs.create_project(project_path, crs=args.crs)

        with gs.setup.init(project_path) as session:
            tools = Tools(
                session=session, capture_output=False, consistent_return_value=True
            )

            try:
                if print_help:
                    return tools.call_cmd([*command, "--help"]).returncode

                if any(item in command for item in SPECIAL_FLAGS):
                    return tools.call_cmd(command).returncode

                return tools.run_cmd(command).returncode

            except subprocess.CalledProcessError as error:
                return error.returncode


def subcommand_create_project(args) -> int:
    try:
        gs.create_project(
            path=args.path,
            crs=args.crs,
            proj4=args.proj4,
            wkt=args.wkt,
            datum=args.datum,
            datum_trans=args.datum_trans,
            description=args.description,
            overwrite=args.overwrite,
        )
    except ScriptError as error:
        print(_("Error creating project: {}").format(error), file=sys.stderr)
        return 1
    return 0


def add_mapset_subparser(subparsers):
    mapset_subparser = subparsers.add_parser("mapset", help="mapset related operations")
    mapset_subparsers = mapset_subparser.add_subparsers(dest="mapset_command")

    subparser = mapset_subparsers.add_parser("create", help="create a new mapset")
    subparser.add_argument("path", help="path to the new mapset")
    subparser.set_defaults(func=subcommand_mapset_create)

    subparser = mapset_subparsers.add_parser("lock", help="lock a mapset")
    subparser.add_argument("mapset_path", type=str)
    subparser.add_argument("--process-id", metavar="PID", type=int, default=1)
    subparser.add_argument("--timeout", metavar="TIMEOUT", type=float, default=30)
    subparser.add_argument("-f", "--force-remove-lock", action="store_true")
    subparser.set_defaults(func=subcommand_lock_mapset)

    subparser = mapset_subparsers.add_parser("unlock", help="unlock a mapset")
    subparser.add_argument("mapset_path", type=str)
    subparser.set_defaults(func=subcommand_unlock_mapset)


def subcommand_mapset_create(args) -> int:
    try:
        create_mapset(args.path)
    except (ScriptError, OSError) as error:
        print(_("Error creating mapset: {}").format(error), file=sys.stderr)
        return 1
    return 0


def subcommand_lock_mapset(args) -> int:
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
        return 1
    return 0


def subcommand_unlock_mapset(args) -> int:
    try:
        unlock_mapset(args.mapset_path)
    except OSError as error:
        print(str(error), file=sys.stderr)
        return 1
    return 0


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


def add_project_subparser(subparsers):
    project_parser = subparsers.add_parser("project", help="project operations")
    project_subparsers = project_parser.add_subparsers(dest="project_command")

    create_parser = project_subparsers.add_parser("create", help="create project")
    create_parser.add_argument("path", help="path to the new project")
    create_parser.add_argument("--crs")
    create_parser.add_argument("--proj4")
    create_parser.add_argument("--wkt")
    create_parser.add_argument("--datum")
    create_parser.add_argument("--datum-trans", dest="datum_trans")
    create_parser.add_argument("--description")
    create_parser.add_argument("--overwrite", action="store_true")
    create_parser.set_defaults(func=subcommand_create_project)


def main(args=None, program=None):
    if program is None:
        program = os.path.basename(sys.argv[0])
        if program == "__main__.py":
            program = f"{Path(sys.executable).name} -m grass.app"

    parser = argparse.ArgumentParser(
        description="Experimental low-level CLI interface to GRASS.",
        prog=program,
    )

    subparsers = parser.add_subparsers(dest="subcommand", required=True)

    run_subparser = subparsers.add_parser(
        "run",
        help="run a tool",
        add_help=False,
    )

    run_subparser.add_argument("tool", type=str, nargs="?", help="name of a tool")
    run_subparser.add_argument("--help", action="store_true")
    run_subparser.add_argument("--crs", type=str)
    run_subparser.add_argument("--project", type=str)
    run_subparser.set_defaults(func=subcommand_run_tool)

    add_project_subparser(subparsers)
    add_mapset_subparser(subparsers)

    subparser = subparsers.add_parser("help")
    subparser.add_argument("page", type=str)
    subparser.set_defaults(func=subcommand_show_help)

    subparser = subparsers.add_parser("man")
    subparser.add_argument("page", type=str)
    subparser.set_defaults(func=subcommand_show_man)

    parsed_args, other_args = parser.parse_known_args(args)

    if parsed_args.subcommand == "run":
        if parsed_args.tool is None and parsed_args.help:
            run_subparser.print_help()
            return 0
        if parsed_args.tool is None:
            run_subparser.print_usage()
            return 2
        return parsed_args.func(parsed_args, other_args, print_help=parsed_args.help)

    return parsed_args.func(parsed_args)
