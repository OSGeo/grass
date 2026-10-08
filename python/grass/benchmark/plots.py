# MODULE:    grass.benchmark
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   Benchmarking for GRASS modules
#
# SPDX-FileCopyrightText: 2021 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later


"""Plotting functionality for benchmark results"""


def get_pyplot(to_file):
    """Get pyplot from matplotlib

    Lazy import to easily run code importing this function on limited installations.
    Only actual call to this function requires matplotlib.

    The *to_file* parameter can be set to True to avoid tkinter dependency
    if the interactive show method is not needed.
    """
    import matplotlib as mpl  # pylint: disable=import-outside-toplevel

    backend = "agg" if to_file else None
    if backend:
        mpl.use(backend)

    import matplotlib.pyplot as plt  # pylint: disable=import-outside-toplevel

    return plt


def nprocs_ranges(result, metric):
    """Return the lowest and highest values of a metric for each nprocs.

    The range of time is the fastest and the slowest run. The range of
    speedup spans the ratios of every serial run to every run with the given
    nprocs; efficiency is that divided by nprocs. The serial runs are those
    with nprocs 1, or with the smallest nprocs when there is no 1. At that
    nprocs, speedup and efficiency are 1 by definition, so the range is
    empty there.

    Returns None when the result has no *all_times*.
    """
    all_times = getattr(result, "all_times", None)
    if not all_times:
        return None
    if metric == "time":
        return [min(t) for t in all_times], [max(t) for t in all_times]
    serial_index = result.nprocs.index(min(result.nprocs))
    serial = all_times[serial_index]
    lows, highs = [], []
    for i, (nprocs, times) in enumerate(zip(result.nprocs, all_times, strict=True)):
        if i == serial_index:
            low = high = 1
        else:
            low, high = min(serial) / max(times), max(serial) / min(times)
        if metric == "efficiency":
            low, high = low / nprocs, high / nprocs
        lows.append(low)
        highs.append(high)
    return lows, highs


def nprocs_plot(
    results, filename=None, title=None, metric="time", figsize=None, dpi=None
):
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
    With it, the range of the runs is shaded around each line in the line's
    color, see :func:`nprocs_ranges`.

    Each result can come with a different list of nprocs, i.e., benchmarks
    which used different values for nprocs can be combined in one plot.

    *figsize* (width and height in inches) and *dpi* are passed to
    matplotlib; the size of a saved image in pixels is their product.
    """
    ylabel = ""
    plt = get_pyplot(to_file=bool(filename))
    _, axes = plt.subplots(figsize=figsize, dpi=dpi)

    x_ticks = set()  # gather x values
    for result in results:
        x = result.nprocs
        x_ticks.update(x)
        if metric == "time":
            values = result.times
            ylabel = "Time [s]"
        elif metric in {"speedup", "efficiency"}:
            values = getattr(result, metric)
            ylabel = metric.title()
        else:
            msg = f"Invalid metric '{metric}' in result, it should be:\
                'time', 'speedup' or 'efficiency'"
            raise ValueError(msg)
        (line,) = plt.plot(x, values, label=result.label)
        ranges = nprocs_ranges(result, metric)
        if ranges:
            plt.fill_between(
                x, *ranges, color=line.get_color(), alpha=0.25, linewidth=0
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
    elif metric == "time":
        plt.title("Execution time by processing elements")
    elif metric in {"speedup", "efficiency"}:
        plt.title(f"{metric.title()} by processing elements")
    if filename:
        plt.savefig(filename)
        plt.close()
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
        x = result.resolutions if show_resolution else result.cells
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
