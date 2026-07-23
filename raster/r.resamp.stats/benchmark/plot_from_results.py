"""Generate benchmark plot from already-collected r.resamp.stats results.

Run this standalone — does NOT require a GRASS session.
Just needs: pip install matplotlib

@author Vinay Chopra
"""

import matplotlib.pyplot as plt
import matplotlib as mpl

mpl.use("Agg")  # no display needed, saves to file

# Data collected from benchmark run (25M cell input, method=average -w)
# Format: coarsening_ratio: (serial_time, best_time, best_nprocs)
summary = {
    "5x": (1.01, 0.28, 10),
    "10x": (0.94, 0.23, 10),
    "15x": (0.95, 0.22, 10),
    "30x": (0.93, 0.23, 9),
}

ratios = list(summary.keys())
speedups = [summary[r][0] / summary[r][1] for r in ratios]
serial_times = [summary[r][0] for r in ratios]
best_times = [summary[r][1] for r in ratios]

fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

# --- Plot 1: Speedup by coarsening ratio ---
bars = ax1.bar(
    ratios,
    speedups,
    color=["#4C72B0", "#DD8452", "#55A868", "#C44E52"],
    edgecolor="white",
    linewidth=0.8,
)
ax1.set_xlabel("Coarsening Ratio (input / output resolution)", fontsize=11)
ax1.set_ylabel("Speedup (serial / best parallel)", fontsize=11)
ax1.set_title(
    "Parallel Speedup by Coarsening Ratio\n(25M cell input, up to 10 threads)",
    fontsize=11,
)
ax1.axhline(y=1.0, color="gray", linestyle="--", linewidth=0.8, label="No speedup (1x)")
ax1.set_ylim(0, 6)

# Annotate bars with speedup value
for bar, sp, r in zip(bars, speedups, ratios, strict=False):
    nprocs = summary[r][2]
    ax1.annotate(
        f"{sp:.2f}x\n({nprocs} threads)",
        xy=(bar.get_x() + bar.get_width() / 2, bar.get_height()),
        xytext=(0, 5),
        textcoords="offset points",
        ha="center",
        va="bottom",
        fontsize=9,
    )

ax1.legend(fontsize=9)

# --- Plot 2: Execution time (serial vs best parallel) ---
x = range(len(ratios))
width = 0.35
bars1 = ax2.bar(
    [i - width / 2 for i in x],
    serial_times,
    width,
    label="Serial (1 thread)",
    color="#4C72B0",
    edgecolor="white",
)
bars2 = ax2.bar(
    [i + width / 2 for i in x],
    best_times,
    width,
    label="Best parallel (9-10 threads)",
    color="#55A868",
    edgecolor="white",
)
ax2.set_xlabel("Coarsening Ratio", fontsize=11)
ax2.set_ylabel("Execution Time [s]", fontsize=11)
ax2.set_title(
    "Execution Time: Serial vs Parallel\n(25M cell input, method=average -w)",
    fontsize=11,
)
ax2.set_xticks(list(x))
ax2.set_xticklabels(ratios)
ax2.legend(fontsize=9)

plt.tight_layout(pad=2.0)
out = "r_resamp_stats_benchmark_nprocs.svg"
plt.savefig(out)
print(f"Plot saved to: {out}")
