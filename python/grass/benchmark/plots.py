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


"""Plotting functionality for benchmark results"""


def get_pyplot(to_file):
    """Get pyplot from matplotlib

    Lazy import to easily run code importing this function on limited installations.
    Only actual call to this function requires matplotlib.

    The *to_file* parameter can be set to True to avoid tkinter dependency
    if the interactive show method is not needed.
    """
    import matplotlib  # pylint: disable=import-outside-toplevel

    if to_file:
        backend = "agg"
    else:
        backend = None
    if backend:
        matplotlib.use(backend)

    import matplotlib.pyplot as plt  # pylint: disable=import-outside-toplevel

    return plt


def nprocs_plot(results, filename=None, title=None, metric="time"):
    """Plot results from a multiple nprocs (thread) benchmarks.

    *results* is a list of individual results from separate benchmarks.
    One result is required to have attributes: *nprocs*, *times*, *label*.
    The *nprocs* attribute is a list of all processing elements
    (cores, threads, processes) used in the benchmark.
    The *times* attribute is a list of corresponding times for each value
    from the *nprocs* list.
    The *label* attribute identifies the benchmark in the legend.

    *metric* can be "time", "speedup", or "efficiency".
    This function plots a corresponding figure based on the chosen metric.

    Optionally, result can have an *all_times* attribute which is a list
    of lists. One sublist is all times recorded for each value of nprocs.

    Each result can come with a different list of nprocs, i.e., benchmarks
    which used different values for nprocs can be combined in one plot.
    """
    ylabel = ""
    plt = get_pyplot(to_file=bool(filename))
    _, axes = plt.subplots()

    x_ticks = set()  # gather x values
    for result in results:
        x = result.nprocs
        x_ticks.update(x)
        if metric == "time":
            mins = [min(i) for i in result.all_times]
            maxes = [max(i) for i in result.all_times]
            plt.plot(x, result.times, label=result.label)
            plt.fill_between(x, mins, maxes, color="gray", alpha=0.3)
            ylabel = "Time [s]"
        elif metric in ["speedup", "efficiency"]:
            ylabel = metric.title()
            plt.plot(x, getattr(result, metric), label=result.label)
        else:
            raise ValueError(
                f"Invalid metric '{metric}' in result, it should be:\
                'time', 'speedup' or 'efficiency'"
            )
    plt.legend()
    # If there is not many x values, show ticks for each, but use default
    # ticks when there is a lot of x values.
    if len(x_ticks) < 10:
        axes.set(xticks=sorted(x_ticks))
    else:
        from matplotlib.ticker import (  # pylint: disable=import-outside-toplevel
            MaxNLocator,
        )

        axes.xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.xlabel("Number of processing elements (cores, threads, processes)")
    plt.ylabel(ylabel)
    if title:
        plt.title(title)
    elif metric == "times":
        plt.title("Execution time by processing elements")
    elif metric in ["speedup", "efficiency"]:
        plt.title(f"{metric.title()} by processing elements")
    if filename:
        plt.savefig(filename)
    else:
        plt.show()


def num_cells_plot(results, filename=None, title=None, show_resolution=False):
    """Plot results from a multiple raster grid size benchmarks.

    *results* is a list of individual results from separate benchmarks
    with one result being similar to the :func:`nprocs_plot` function.
    The result is required to have *times* and *label* attributes
    and may have an *all_times* attribute.
    Further, it is required to have *cells* attribute, or,
    when ``show_resolution=True``, it needs to have a *resolutions* attribute.

    Each result can come with a different list of nprocs, i.e., benchmarks
    which used different values for nprocs can be combined in one plot.
    """
    plt = get_pyplot(to_file=bool(filename))
    axes = plt.gca()
    if show_resolution:
        axes.invert_xaxis()

    x_ticks = set()
    for result in results:
        if show_resolution:
            x = result.resolutions
        else:
            x = result.cells
        x_ticks.update(x)
        plt.plot(x, result.times, label=result.label)
        if hasattr(result, "all_times"):
            mins = [min(i) for i in result.all_times]
            maxes = [max(i) for i in result.all_times]
            plt.fill_between(x, mins, maxes, color="gray", alpha=0.3)

    plt.legend()
    axes.set(xticks=sorted(x_ticks))
    if not show_resolution:
        axes.ticklabel_format(axis="x", style="scientific", scilimits=(0, 0))
    if show_resolution:
        plt.xlabel("Resolution [map units]")
    else:
        plt.xlabel("Number of cells")
    plt.ylabel("Time [s]")
    if title:
        plt.title(title)
    elif show_resolution:
        plt.title("Execution time by resolution")
    else:
        plt.title("Execution time by cell count")
    if filename:
        plt.savefig(filename)
    else:
        plt.show()
