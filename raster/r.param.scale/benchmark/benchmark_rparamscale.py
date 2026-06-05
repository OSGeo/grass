#!/usr/bin/env python3
#
# benchmark.py - parallel scaling benchmark for r.param.scale
#
# WHAT THIS MEASURES
#   Wall-clock parallel SPEEDUP (median time at 1 thread / median time at N threads)
#   for r.param.scale on a fixed input raster. For each window size it runs every
#   thread count, takes the median of the timed runs (one warmup is discarded), and
#   reports the speedup of each thread count against that size's own 1-thread run.
#   Defaults:
#       input   = synth_dem_100m   (generate it with the companion DEM script)
#       method  = elev
#       memory  = 300 MB
#       sizes   = [31]             (window side; add more to sweep window size)
#       nprocs  = [1, 2, 4, 8]
#       repeat  = 4                (1 warmup discarded + 3 timed, median reported)
#   A config whose timed runs spread more than 5% is flagged. Occasional flags are
#   EXPECTED on machines with variable thermals or scheduling; a flag just means that
#   config should be re-run, not that anything is wrong.
#
#   The script creates one temporary output raster
#   (tmp_benchmark_rparamscale_delete_me) and removes it at the end, even on error.
#
# RESULTS ARE HARDWARE-DEPENDENT. Absolute times and the speedup curve depend on
# core count and type (performance vs efficiency cores), memory bandwidth, cooling,
# and power source. Your numbers will differ from anyone else's. Use AC power.
#
# THREAD PLACEMENT ON HYBRID CPUs
#   On CPUs with both performance and efficiency cores (e.g. Apple M-series), thread
#   counts below the performance-core count can show large run-to-run variance
#   because the scheduler places threads on P or E cores inconsistently. Pin the
#   threads to cores to stabilize those configs:
#       OMP_PROC_BIND=true OMP_PLACES=cores grass <...> --exec python3 benchmark.py
#
# RUN
#   grass /path/to/grassdata/<location>/<mapset> --exec python3 benchmark.py
#   (the input raster must exist in that mapset)

import io
import os
import platform
import statistics
import subprocess
from contextlib import redirect_stdout

import grass.script as gs
from grass.pygrass.modules import Module
from grass.benchmark import benchmark_single

# ---- configuration -------------------------------------------------------
INPUT = "synth_dem_100m"
METHOD = "elev"
MEMORY = 300  # MB, passed to memory=
SIZES = [31]  # window sides to test (e.g. [5, 15, 31, 51] to sweep)
NPROCS = [1, 2, 4, 8]  # thread counts to test (1 is the baseline)
REPEAT = 4  # 1 warmup (discarded) + (REPEAT - 1) timed runs
SPREAD_FLAG = 5.0  # percent; flag a config whose timed runs spread above this
TMP_OUTPUT = "tmp_benchmark_rparamscale_delete_me"  # created then removed
# --------------------------------------------------------------------------


def keep_awake():
    """On macOS, hold an idle-sleep assertion for this process's lifetime."""
    if platform.system() == "Darwin":
        try:
            return subprocess.Popen(["caffeinate", "-i", "-w", str(os.getpid())])
        except FileNotFoundError:
            pass
    return None


def report_power():
    if platform.system() == "Darwin":
        try:
            ps = subprocess.check_output(["pmset", "-g", "ps"], text=True)
            if "Battery Power" in ps:
                print("Power: BATTERY -- WARNING: plug into AC, results will be noisy")
            else:
                print("Power: AC")
        except Exception:
            print("Power: could not determine")
    else:
        print("Power: not checked (non-macOS); ensure AC power and no power saving")


def timed_median(all_times):
    """Discard the warmup (first run); return (median, spread_percent) of the rest."""
    timed = all_times[1:]
    med = statistics.median(timed)
    spread = (max(timed) - min(timed)) / med * 100.0 if med else 0.0
    return med, spread


def bench(size, nprocs):
    mod = Module(
        "r.param.scale",
        input=INPUT,
        output=TMP_OUTPUT,
        method=METHOD,
        size=size,
        memory=MEMORY,
        nprocs=nprocs,
        overwrite=True,
        quiet=True,
        stderr_=subprocess.PIPE,
        run_=False,
    )
    # benchmark_single prints raw per-run times; silence it and keep only the median.
    with redirect_stdout(io.StringIO()):
        res = benchmark_single(mod, label="size%d n%d" % (size, nprocs), repeat=REPEAT)
    return timed_median(res.all_times)


def main():
    awake = keep_awake()
    try:
        print("GRASS %s" % gs.version()["version"])
        print("Machine: %s; %d logical CPUs" % (platform.platform(), os.cpu_count()))
        report_power()
        print(
            "Workload: %s, method=%s, memory=%d MB, sizes=%s, nprocs=%s, "
            "1 warmup + %d timed runs (median)"
            % (INPUT, METHOD, MEMORY, SIZES, NPROCS, REPEAT - 1)
        )
        print(
            "Pinning: OMP_PROC_BIND=%s OMP_PLACES=%s"
            % (
                os.environ.get("OMP_PROC_BIND", "unset"),
                os.environ.get("OMP_PLACES", "unset"),
            )
        )
        gs.run_command("g.region", raster=INPUT)

        for size in SIZES:
            results = {p: bench(size, p) for p in NPROCS}
            base = results[NPROCS[0]][0]
            print(
                "\nr.param.scale size=%d  (%s, memory=%d MB, baseline = %d thread)"
                % (size, INPUT, MEMORY, NPROCS[0])
            )
            print("  nprocs | median time (s) | speedup")
            print("  -------+-----------------+---------")
            for p in NPROCS:
                med, spread = results[p]
                flag = " *" if spread > SPREAD_FLAG else ""
                print("    %d   |   %11.3f   |  %5.2fx%s" % (p, med, base / med, flag))
            flagged = [str(p) for p in NPROCS if results[p][1] > SPREAD_FLAG]
            if flagged:
                print(
                    "  * timed-run spread > %.0f%% (expected on thermally/"
                    "scheduling-variable machines; just re-run that config): "
                    "nprocs %s" % (SPREAD_FLAG, ", ".join(flagged))
                )
    finally:
        # Always remove the temporary output raster, even on error.
        try:
            gs.run_command(
                "g.remove", type="raster", name=TMP_OUTPUT, flags="f", quiet=True
            )
        except Exception:
            pass
        if awake:
            awake.terminate()


if __name__ == "__main__":
    main()
