# MODULE:    grass.benchmark
#
# AUTHOR(S): Aaron Saw Min Sern
#            Vaclav Petras
#
# PURPOSE:   Benchmarking for GRASS GIS modules
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.


"""Plotting functionality for benchmark results"""


def nprocs_plot(results, filename=None):
    # Lazy import to easily run f on limited installations.
    # Only actual call to this function requires matplotlib.
    import matplotlib.pyplot as plt

    for result in results:
        plt.plot(result.threads, result.avg_times, label=result.label)
        mins = [min(i) for i in result.all_times]
        maxes = [max(i) for i in result.all_times]
        plt.fill_between(result.threads, mins, maxes, color="gray", alpha=0.3)
    plt.legend()
    if filename:
        plt.savefig(filename)
    else:
        plt.show()


def resolutions_plot(results, filename=None):
    import matplotlib.pyplot as plt

    for result in results:
        fig, ax = plt.subplots()
        ax.invert_xaxis()
        plt.plot(result.resolutions, result.avg_times, label=result.label)
        mins = [min(i) for i in result.all_times]
        maxes = [max(i) for i in result.all_times]
        plt.fill_between(result.resolutions, mins, maxes, color="gray", alpha=0.3)
    plt.legend()
    if filename:
        plt.savefig(filename)
    else:
        plt.show()
