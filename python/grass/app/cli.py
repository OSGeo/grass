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

# Special flags supported besides help and --json which does not need special handling:
SPECIAL_FLAGS = [
    "--interface-description",
    "--md-description",
    "--wps-process-description",
    "--script",
]
# To make this list shorter, we don't support outdated special flags:
# --help-text --html-description --rst-description


def subcommand_run_tool(args, tool_args: list, print_help: bool) -> int:
    command = [args.tool, *tool_args]
    with ExitStack() as stack:
        if args.project:
            project_path = Path(args.project)
        else:
            tmp_dir_name = stack.enter_context(tempfile.TemporaryDirectory())
            project_name = "project"
            project_path = Path(tmp_dir_name) / project_name
            gs.create_project(project_path, crs=args.crs)

        with gs.setup.init(project_path) as session:
            if args.import_vector:
                fpath = Path(args.import_vector)
                try:
                    gs.run_command("v.import", input=str(fpath), output=fpath.stem)
                except ScriptError as e:
                    print(f"Error importing vector: {e}", file=sys.stderr)
                    return 1

            if args.link_vector:
                fpath = Path(args.link_vector)
                try:
                    gs.run_command("v.external", input=str(fpath), output=fpath.stem)
                except ScriptError as e:
                    print(f"Error linking vector: {e}", file=sys.stderr)
                    return 1

            if args.link_output:
                out_dir = Path(args.link_output)
                try:
                    gs.run_command(
                        "r.external.out", directory=str(out_dir), format="GTiff"
                    )
                except ScriptError as e:
                    print(f"Error setting external output: {e}", file=sys.stderr)
                    return 1

            tools = Tools(
                session=session, capture_output=False, consistent_return_value=True
            )

            return_code = 0

            try:
                if print_help:
                    return tools.call_cmd([*command, "--help"]).returncode
                elif any(item in command for item in SPECIAL_FLAGS):
                    return tools.call_cmd(command).returncode
                else:
                    return_code = tools.run_cmd(command).returncode

            except subprocess.CalledProcessError as error:
                return_code = error.returncode

            if return_code == 0:
                if args.export_raster:
                    if "=" in args.export_raster:
                        map_name, out_file = args.export_raster.split("=", 1)
                        try:
                            gs.run_command(
                                "r.out.gdal", input=map_name, output=out_file
                            )
                        except ScriptError as e:
                            print(f"Error exporting raster: {e}", file=sys.stderr)
                            return 1
                    else:
                        print(
                            "Error: --export-raster format must be 'map=file'",
                            file=sys.stderr,
                        )

                if args.export_vector:
                    if "=" in args.export_vector:
                        map_name, out_file = args.export_vector.split("=", 1)
                        try:
                            gs.run_command("v.out.ogr", input=map_name, output=out_file)
                        except ScriptError as e:
                            print(f"Error exporting vector: {e}", file=sys.stderr)
                            return 1
                    else:
                        print(
                            "Error: --export-vector format must be 'map=file'",
                            file=sys.stderr,
                        )

            return return_code


def subcommand_create_project(args) -> int:
    """Translates args to function parameters"""
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
    create_parser.add_argument(
        "--crs",
        help="CRS for the project",
    )
    create_parser.add_argument(
        "--proj4",
        help="if given, create new project based on Proj4 definition",
    )
    create_parser.add_argument(
        "--wkt",
        help=(
            "if given, create new project based on WKT definition "
            "(can be path to PRJ file or WKT string)"
        ),
    )
    create_parser.add_argument(
        "--datum",
        help="GRASS format datum code",
    )
    create_parser.add_argument(
        "--datum-trans",
        dest="datum_trans",
        help="datum transformation parameters (used for epsg and proj4)",
    )
    create_parser.add_argument(
        "--description",
        help="description of the project",
    )
    create_parser.add_argument(
        "--overwrite",
        action="store_true",
        help="overwrite existing project",
    )
    create_parser.set_defaults(func=subcommand_create_project)


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
    run_subparser.add_argument("--crs", type=str, help="CRS to use for computations")
    run_subparser.add_argument(
        "--project", type=str, help="project to use for computations"
    )

    run_subparser.add_argument(
        "--import-vector",
        type=str,
        help="Import a vector file (v.import) before execution",
    )
    run_subparser.add_argument(
        "--link-vector",
        type=str,
        help="Link a vector file (v.external) before execution",
    )

    run_subparser.add_argument(
        "--export-raster",
        type=str,
        help='Export a raster map after execution (e.g. --export-raster="map=file.tif")',
    )
    run_subparser.add_argument(
        "--export-vector",
        type=str,
        help='Export a vector map after execution (e.g. --export-vector="map=file.gpkg")',
    )

    run_subparser.add_argument(
        "--link-output",
        type=str,
        help="Direct raster output to an external directory (r.external.out)",
    )
    run_subparser.set_defaults(func=subcommand_run_tool)

    add_project_subparser(subparsers)
    add_mapset_subparser(subparsers)

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
