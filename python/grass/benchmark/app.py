# MODULE:    grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.


"""CLI for the benchmark package"""

import argparse
import sys
from pathlib import Path

from grass.benchmark import (
    join_results_from_files,
    load_results_from_file,
    num_cells_plot,
    save_results_to_file,
)


def join_results_cli(args):
    """Translate CLI parser result to API calls."""
    results = join_results_from_files(
        source_filenames=args.results,
        prefixes=args.prefixes,
    )
    save_results_to_file(results, args.output)


def plot_cells_cli(args):
    """Translate CLI parser result to API calls."""
    results = load_results_from_file(args.input)
    num_cells_plot(
        results.results,
        filename=args.output,
        title=args.title,
        show_resolution=args.resolutions,
    )


def get_executable_name():
    """Get name of the executable and module.

    This is a workaround for Python issue:
    argparse support for "python -m module" in help
    https://bugs.python.org/issue22240
    """
    executable = Path(sys.executable).stem
    return f"{executable} -m grass.benchmark"


def add_subcommand_parser(subparsers, name, description):
    """Add parser for a subcommand into subparsers."""
    # help is in parent's help, description in subcommand's help.
    return subparsers.add_parser(name, help=description, description=description)


def add_subparsers(parser, dest):
    """Add subparsers in a unified way.

    Uses title 'subcommands' for the list of commands
    (instead of the 'positional' which is the default).

    The *dest* should be 'command', 'subcommand', etc. with appropriate nesting.
    """
    if sys.version_info < (3, 7):
        # required as parameter is only in >=3.7.
        return parser.add_subparsers(title="subcommands", dest=dest)
    return parser.add_subparsers(title="subcommands", required=True, dest=dest)


def add_results_subcommand(parent_subparsers):
    """Add results subcommand."""
    main_parser = add_subcommand_parser(
        parent_subparsers, "results", description="Manipulate results"
    )
    main_subparsers = add_subparsers(main_parser, dest="subcommand")

    join = main_subparsers.add_parser("join", help="Join results")
    join.add_argument("results", help="Files with results", nargs="*", metavar="file")
    join.add_argument("output", help="Output file", metavar="output_file")
    join.add_argument(
        "--prefixes",
        help="Add prefixes to result labels per file",
        action="extend",
        nargs="*",
        metavar="text",
    )
    join.set_defaults(handler=join_results_cli)


def add_plot_subcommand(parent_subparsers):
    """Add plot subcommand."""
    main_parser = add_subcommand_parser(
        parent_subparsers, "plot", description="Plot results"
    )
    main_subparsers = add_subparsers(main_parser, dest="subcommand")

    join = main_subparsers.add_parser("cells", help="Plot for variable number of cells")
    join.add_argument("input", help="file with results (JSON)", metavar="input_file")
    join.add_argument(
        "output", help="output file (e.g., PNG)", nargs="?", metavar="output_file"
    )
    join.add_argument(
        "--title",
        help="Title for the plot",
        metavar="text",
    )
    join.add_argument(
        "--resolutions",
        help="Use resolutions for x axis instead of cell count",
        action="store_true",
    )
    join.set_defaults(handler=plot_cells_cli)


def define_arguments():
    """Define top level parser and create subparsers."""
    parser = argparse.ArgumentParser(
        description="Process results from module benchmarks.",
        prog=get_executable_name(),
    )
    subparsers = add_subparsers(parser, dest="command")

    add_results_subcommand(subparsers)
    add_plot_subcommand(subparsers)

    return parser


def main(args=None):
    """Define and parse command line parameters then run the appropriate handler."""
    parser = define_arguments()
    args = parser.parse_args(args)
    args.handler(args)


if __name__ == "__main__":
    main()
