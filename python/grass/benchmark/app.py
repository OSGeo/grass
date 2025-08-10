# MODULE:    grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS modules
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
    nprocs_plot,
    num_cells_plot,
    save_results_to_file,
)


class CliUsageError(ValueError):
    """Raised when error is in the command line arguments.

    Used when the error is discovered only after argparse parsed the arguments.
    """

    # ArgumentError from argparse may work too, but it is not documented and
    # takes a reference argument which we don't have access to after the parse step.


def join_results_cli(args):
    """Translate CLI parser result to API calls."""
    if args.prefixes and len(args.results) != len(args.prefixes):
        msg = (
            f"Number of prefixes ({len(args.prefixes)}) needs to be the same"
            f" as the number of input result files ({len(args.results)})"
        )
        raise CliUsageError(msg)

    def select_only(result):
        return result.label == args.only

    select_function = select_only if args.only else None
    results = join_results_from_files(
        source_filenames=args.results,
        prefixes=args.prefixes,
        select=select_function,
        prefixes_as_labels=args.re_label,
    )

    save_results_to_file(results, args.output)


def plot_nprocs_cli(args):
    """Translate CLI parser result to API calls."""
    results = load_results_from_file(args.input)
    nprocs_plot(
        results.results,
        filename=args.output,
        title=args.title,
        metric=args.metric,
    )


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


class ExtendAction(argparse.Action):
    """Support for agrparse action="extend" before Python 3.8

    Each parser instance needs the action to be registered.
    """

    # pylint: disable=too-few-public-methods
    def __call__(self, parser, namespace, values, option_string=None):
        items = getattr(namespace, self.dest) or []
        items.extend(values)
        setattr(namespace, self.dest, items)


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
    if sys.version_info < (3, 8):
        join.register("action", "extend", ExtendAction)
    join.add_argument(
        "--prefixes",
        help="Add prefixes to result labels per file",
        action="extend",
        nargs="*",
        metavar="text",
    )
    join.add_argument(
        "--only",
        help="Select only results with matching label",
        metavar="label",
    )
    join.add_argument(
        "--re-label",
        help="Use prefixes as the new labels",
        action="store_true",
    )
    join.set_defaults(handler=join_results_cli)


def add_plot_io_arguments(parser):
    """Add input and output arguments to *parser*."""
    parser.add_argument(
        "input", help="file with results (e.g. results.json)", metavar="input_file"
    )
    parser.add_argument(
        "output",
        help=(
            "output file with extension (e.g., figure.png)."
            " If not provided, the plot will be opened in a new window."
        ),
        nargs="?",
        metavar="output_file",
    )


def add_plot_title_argument(parser):
    """Add title argument to *parser*."""
    parser.add_argument(
        "--title",
        help="Title for the plot",
        metavar="text",
    )


def add_plot_metric_argument(parser):
    """Add metric argument to *parser*."""
    parser.add_argument(
        "--metric",
        help="Metric for the plot (default: time)",
        default="time",
        choices=["time", "speedup", "efficiency"],
    )


def add_plot_subcommand(parent_subparsers):
    """Add plot subcommand."""
    main_parser = add_subcommand_parser(
        parent_subparsers, "plot", description="Plot results"
    )
    main_subparsers = add_subparsers(main_parser, dest="subcommand")

    join = main_subparsers.add_parser("cells", help="Plot for variable number of cells")
    add_plot_io_arguments(join)
    add_plot_title_argument(join)
    join.add_argument(
        "--resolutions",
        help="Use resolutions for x axis instead of cell count",
        action="store_true",
    )
    join.set_defaults(handler=plot_cells_cli)

    nprocs = main_subparsers.add_parser(
        "nprocs", help="Plot for variable number of processing elements"
    )
    add_plot_io_arguments(nprocs)
    add_plot_title_argument(nprocs)
    add_plot_metric_argument(nprocs)
    nprocs.set_defaults(handler=plot_nprocs_cli)


def define_arguments():
    """Define top level parser and create subparsers."""
    parser = argparse.ArgumentParser(
        description="Process results from module benchmarks.",
        prog=get_executable_name(),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    subparsers = add_subparsers(parser, dest="command")

    add_results_subcommand(subparsers)
    add_plot_subcommand(subparsers)

    return parser


def main(args=None):
    """Define and parse command line parameters then run the appropriate handler."""
    parser = define_arguments()
    args = parser.parse_args(args)
    try:
        args.handler(args)
    except CliUsageError as error:
        # Report a usage error and exit.
        sys.exit(f"ERROR: {error}")


if __name__ == "__main__":
    main()
