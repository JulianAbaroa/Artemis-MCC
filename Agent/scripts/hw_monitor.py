"""
hw_monitor.py
Logs the laptop's thermal and clock state while a run plays, to see thermal throttling next to the training speed (seconds per
update). One PowerShell process samples every --interval seconds, without admin rights:
  cpu_perf   % Processor Performance of _Total (effective speed against the base clock: above 100 = turbo; it falls when the CPU
             throttles), cpu_util (% Processor Utility), cpu_mhz
  tz_passive lowest PercentPassiveLimit of the ACPI thermal zones (100 = no passive cooling; lower = Windows is slowing the CPU for
             heat), tz_reasons (OR of their ThrottleReasons), tz_c (hottest zone in C; many laptops report a fixed value)
  gpu_*      nvidia-smi: temperature, graphics clock, power, utilization, encoder utilization (OBS NVENC) and the clock event reasons
             bitmask (0x20 / 0x40 thermal slowdown, 0x4 power cap, 0x1 idle)
Writes Agent/runs/hw/<name>.csv (epoch time first). Stops after --max-seconds or when Agent/runs/hw/<name>.stop exists.
python Tools/game.py py Agent/scripts/hw_monitor.py --name hitoa_noche [--interval 15] [--max-seconds 36000] --no-wait
python Tools/game.py py Agent/scripts/hw_monitor.py --summary hitoa_noche [--run hitoa_noche] [--bin 30]
    per bin of --bin minutes: CPU speed and use, passive limit, GPU temperature, clock and reasons, and with --run the mean
    collect seconds per update of Agent/runs/<run>/metrics.jsonl in the same minutes.
"""
import argparse
import csv
import json
import os
import subprocess
import sys
import time

RUNS = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "runs")
HW = os.path.join(RUNS, "hw")
FIELDS = ["time", "cpu_perf", "cpu_util", "cpu_mhz", "tz_passive", "tz_reasons", "tz_c",
          "gpu_c", "gpu_mhz", "gpu_w", "gpu_util", "gpu_enc", "gpu_reasons"]
GPU_THERMAL = 0x20 | 0x40 | 0x8

LOOP = r"""
$ErrorActionPreference = 'SilentlyContinue'
while ($true) {
    $p = Get-CimInstance Win32_PerfFormattedData_Counters_ProcessorInformation -Filter "Name='_Total'"
    $zones = @(Get-CimInstance Win32_PerfFormattedData_Counters_ThermalZoneInformation)
    $passive = 100; $reasons = 0; $temp = 0
    foreach ($z in $zones) {
        if ($z.PercentPassiveLimit -lt $passive) { $passive = $z.PercentPassiveLimit }
        $reasons = $reasons -bor $z.ThrottleReasons
        $c = $z.HighPrecisionTemperature / 10.0 - 273.15
        if ($c -gt $temp) { $temp = $c }
    }
    $g = (& nvidia-smi --query-gpu=temperature.gpu,clocks.gr,power.draw,utilization.gpu,utilization.encoder,clocks_event_reasons.active --format=csv,noheader,nounits) -replace '\s', ''
    if (-not $g) { $g = ',,,,,' }
    $inv = [System.Globalization.CultureInfo]::InvariantCulture
    [string]::Format($inv, '{0},{1},{2},{3},{4},{5:0.0},{6}', $p.PercentProcessorPerformance, $p.PercentProcessorUtility,
        $p.ProcessorFrequency, $passive, $reasons, $temp, $g)
    Start-Sleep -Seconds INTERVAL
}
"""


def monitor(args):
    os.makedirs(HW, exist_ok=True)
    path = os.path.join(HW, args.name + ".csv")
    stop = os.path.join(HW, args.name + ".stop")
    if os.path.exists(stop):
        os.remove(stop)
    loop = LOOP.replace("INTERVAL", str(args.interval))
    proc = subprocess.Popen(["powershell", "-NoProfile", "-Command", loop], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                            text=True, bufsize=1, creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    start = time.time()
    rows = 0
    new = not os.path.exists(path)
    with open(path, "a", newline="", encoding="utf-8") as f:
        out = csv.writer(f)
        if new:
            out.writerow(FIELDS)
        print("hw_monitor: writing %s every %d s (stop: %s)" % (path, args.interval, stop), flush=True)
        try:
            for line in proc.stdout:
                parts = line.strip().split(",")
                if len(parts) == len(FIELDS) - 1:
                    out.writerow(["%.1f" % time.time()] + parts)
                    f.flush()
                    rows += 1
                elif rows == 0:
                    print("hw_monitor: unexpected line %r" % line.strip(), flush=True)
                if time.time() - start > args.max_seconds or os.path.exists(stop):
                    break
        finally:
            proc.kill()
    print("hw_monitor: %d samples in %.0f s" % (rows, time.time() - start))


def num(value):
    try:
        return float(value)
    except (TypeError, ValueError):
        return None


def mean(values):
    values = [v for v in values if v is not None]
    return sum(values) / len(values) if values else None


def fmt(value, spec="%.0f"):
    return "-" if value is None else spec % value


def summary(args):
    path = os.path.join(HW, args.summary + ".csv")
    with open(path, encoding="utf-8") as f:
        samples = list(csv.DictReader(f))
    if not samples:
        print("no samples in " + path)
        return
    updates = []
    if args.run:
        with open(os.path.join(RUNS, args.run, "metrics.jsonl"), encoding="utf-8") as f:
            for line in f:
                row = json.loads(line)
                if row.get("kind") == "update":
                    updates.append((row["time"], row.get("collect")))
    t0 = float(samples[0]["time"])
    width = args.bin * 60
    bins = {}
    for s in samples:
        bins.setdefault(int((float(s["time"]) - t0) // width), []).append(s)
    print("%-7s %4s | %8s %5s %4s %8s %5s | %5s %5s %6s %4s %4s %-12s | %6s %4s" % (
        "min", "n", "cpu_perf", "p10", "use", "passive", "tzR", "gpuC", "max", "gpuMHz", "enc", "W", "gpu_reasons", "s/upd", "upd"))
    for b in sorted(bins):
        rows = bins[b]
        perf = sorted(v for v in (num(r["cpu_perf"]) for r in rows) if v is not None)
        gpu_c = [num(r["gpu_c"]) for r in rows]
        reasons = 0
        for r in rows:
            try:
                reasons |= int(r["gpu_reasons"], 16)
            except ValueError:
                pass
        tz_r = 0
        for r in rows:
            tz_r |= int(num(r["tz_reasons"]) or 0)
        lo, hi = t0 + b * width, t0 + (b + 1) * width
        collect = [c for t, c in updates if lo <= t < hi]
        flag = " THERMAL" if reasons & GPU_THERMAL else ""
        print("%-7s %4d | %8s %5s %4s %8s %5s | %5s %5s %6s %4s %4s %-12s | %6s %4s" % (
            "%d-%d" % (b * args.bin, (b + 1) * args.bin), len(rows),
            fmt(mean(perf)), fmt(perf[len(perf) // 10] if perf else None), fmt(mean([num(r["cpu_util"]) for r in rows])),
            fmt(min((num(r["tz_passive"]) for r in rows if num(r["tz_passive"]) is not None), default=None)), hex(tz_r),
            fmt(mean(gpu_c)), fmt(max((c for c in gpu_c if c is not None), default=None)),
            fmt(mean([num(r["gpu_mhz"]) for r in rows])), fmt(mean([num(r["gpu_enc"]) for r in rows])),
            fmt(mean([num(r["gpu_w"]) for r in rows])), hex(reasons) + flag,
            fmt(mean(collect), "%.2f"), len(collect)))
    print("cpu_perf: % of the base clock (2.4 GHz on the i7-13620H; turbo is above 100). passive < 100 or tzR != 0: Windows "
          "throttles the CPU for heat. GPU THERMAL: 0x8 / 0x20 / 0x40 in the reasons.")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--name", help="monitor: name of the CSV in Agent/runs/hw/")
    parser.add_argument("--interval", type=int, default=15)
    parser.add_argument("--max-seconds", type=float, default=36000)
    parser.add_argument("--summary", help="summarize Agent/runs/hw/<name>.csv")
    parser.add_argument("--run", help="summary: training run whose metrics.jsonl to align")
    parser.add_argument("--bin", type=int, default=30, help="summary: minutes per row")
    args = parser.parse_args()
    if args.summary:
        summary(args)
    elif args.name:
        monitor(args)
    else:
        parser.error("--name or --summary")
    return 0


if __name__ == "__main__":
    sys.exit(main())
