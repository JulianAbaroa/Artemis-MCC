#!/usr/bin/env python3
"""Resume las lineas [Profiler] del viewer: promedios por pass, fps y separacion por escenario.

Uso:
    python profiler_stats.py log.txt [log2.txt ...]
    python profiler_stats.py log.txt --split-aim 0.5   # umbral (ms) de CPU aim para separar escenarios
"""
import argparse
import re
import statistics as st
import sys

LINE = re.compile(r"\[Profiler\]\s+(\d+)\s+fps")
CPU = re.compile(r"([A-Za-z()]+)\s+([\d.]+)/([\d.]+);")
GPU = re.compile(r"([A-Za-z]+)\s+([\d.]+);")
COUNT = re.compile(r"(\w+)=(\d+)")


def parse(path):
    rows = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for raw in f:
            m = LINE.search(raw)
            if not m:
                continue
            text = raw[m.end():]
            text, _, count_part = text.partition("| count")
            cpu_part, _, gpu_part = text.partition("| GPU")
            cpu = {n.lower(): (float(a), float(b)) for n, a, b in CPU.findall(cpu_part)}
            gpu = {n.lower(): float(v) for n, v in GPU.findall(gpu_part)}
            counts = {n: int(v) for n, v in COUNT.findall(count_part)}
            rows.append({"fps": int(m.group(1)), "cpu": cpu, "gpu": gpu, "count": counts})
    return rows


def summarize(title, rows):
    if not rows:
        return
    fps = [r["fps"] for r in rows]
    print(f"\n=== {title}: {len(rows)} muestras (1 s c/u) ===")
    print(f"fps: media {st.mean(fps):.1f} | min {min(fps)} | p5 {sorted(fps)[max(0, int(len(fps) * 0.05) - 1)]}"
          f" | <60: {sum(f < 60 for f in fps)} ({100 * sum(f < 60 for f in fps) / len(fps):.0f}%)"
          f" | <50: {sum(f < 50 for f in fps)}")

    print("\nCPU (ms)            avg(media)  max(media)  max(peor)")
    for name in rows[0]["cpu"]:
        avgs = [r["cpu"][name][0] for r in rows if name in r["cpu"]]
        maxs = [r["cpu"][name][1] for r in rows if name in r["cpu"]]
        print(f"  {name:<16}{st.mean(avgs):>10.2f}{st.mean(maxs):>12.2f}{max(maxs):>11.2f}")

    gpu_rows = [r for r in rows if r["gpu"]]
    if gpu_rows:
        print("\nGPU (ms)            avg(media)  avg(peor)")
        for name in gpu_rows[0]["gpu"]:
            vals = [r["gpu"][name] for r in gpu_rows if name in r["gpu"]]
            print(f"  {name:<16}{st.mean(vals):>10.2f}{max(vals):>11.2f}")
    _counts(rows)


def _counts(rows):
    keys = sorted({k for r in rows for k in r.get("count", {})})
    if not keys:
        return
    print("\nTamanos (ultimo valor de cada segundo)  media / max")
    for k in keys:
        vals = [r["count"][k] for r in rows if k in r.get("count", {})]
        print(f"  {k:<16}{st.mean(vals):>12.0f}{max(vals):>12}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--worst", type=int, default=0,
                    help="muestra los N segundos con menos fps (indice, fps, frame max, dynamic max, tamanos)")
    ap.add_argument("--split-aim", type=float, default=0.5,
                    help="CPU aim avg (ms) sobre el cual se considera 'sin seleccion' (aims dibujados)")
    args = ap.parse_args()

    rows = []
    for p in args.files:
        rows += parse(p)
    if not rows:
        sys.exit("No encontre lineas [Profiler].")

    summarize("TODO", rows)

    if args.worst > 0:
        print(f"\n=== {args.worst} peores segundos (indice en el log, empezando en 1) ===")
        ranked = sorted(enumerate(rows, 1), key=lambda item: item[1]["fps"])[:args.worst]
        for idx, r in ranked:
            frame = r["cpu"].get("frame", (0, 0))
            dyn = r["cpu"].get("dynamic", (0, 0))
            print(f"  #{idx:<4} {r['fps']:>3} fps | frame avg/max {frame[0]:.2f}/{frame[1]:.2f} | "
                  f"dynamic avg/max {dyn[0]:.2f}/{dyn[1]:.2f} | {r.get('count', {})}")

    heavy = [r for r in rows if r["cpu"].get("aim", (0, 0))[0] >= args.split_aim]
    light = [r for r in rows if r["cpu"].get("aim", (0, 0))[0] < args.split_aim]
    summarize(f"AIMS ACTIVOS (cpu aim >= {args.split_aim} ms)", heavy)
    summarize(f"AIMS INACTIVOS (cpu aim < {args.split_aim} ms)", light)


if __name__ == "__main__":
    main()
