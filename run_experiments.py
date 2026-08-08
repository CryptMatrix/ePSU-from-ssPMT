#!/usr/bin/env python3
"""
Experiment data collection script for ePSU-from-ssPMT.

Runs fig22 (mqRPMT) and fig24 (PSU) experiments across:
  - 2 schemes: fast, low
  - 3 network settings: LAN, WAN_100Mbps, WAN_10Mbps
  - 5 set sizes: nn = 14, 16, 18, 20, 22

Collects communication overhead (MB) and total protocol computation time (ms),
then outputs results as CSV files.

Usage:
    sudo python3 run_experiments.py [--fig22] [--fig24] [--output-dir DIR]
"""

import argparse
import csv
import os
import re
import subprocess
import sys
import time
from pathlib import Path

# ──────────────────────────────────────────────────────────
# Configuration
# ──────────────────────────────────────────────────────────

PROJECT_ROOT = Path(__file__).resolve().parent

SCHEMES = {
    "fast": {
        "dir": PROJECT_ROOT / "ePSU_fast",
        "rpmt_bin": "build/RPMT",
        "psu_bin": "build/PSU",
        "rpmt_extra_args": [],
        "psu_extra_args": [],
    },
    "low": {
        "dir": PROJECT_ROOT / "ePSU_low",
        "rpmt_bin": "build/RPMT",
        "psu_bin": "build/PSU",
        "rpmt_extra_args": ["-nt", "1"],
        "psu_extra_args": ["-nt", "1"],
    },
}

NET_CONFIGS = [
    ("LAN", "10gbit", "100us", ""),
    ("WAN_100Mbps", "100mbit", "40ms", "limit 20000"),
    ("WAN_10Mbps", "10mbit", "40ms", "limit 20000"),
]

NN_LIST = [14, 16, 18, 20, 22]


# ──────────────────────────────────────────────────────────
# Output parsing
# ──────────────────────────────────────────────────────────

def parse_output(stdout: str) -> dict:
    """
    Parse the TOTAL and WALL CLOCK lines from protocol output.

    Expected format:
      [TOTAL]          653.954 ms |      6.249 MB
      [WALL CLOCK]     655.047 ms
    """
    result = {}
    for line in stdout.splitlines():
        m = re.match(r'\s*\[TOTAL\]\s+([\d.]+)\s+ms\s*\|\s*([\d.]+)\s+MB', line)
        if m:
            result["total_time_ms"] = float(m.group(1))
            result["total_comm_mb"] = float(m.group(2))
        m = re.match(r'\s*\[WALL CLOCK\]\s+([\d.]+)\s+ms', line)
        if m:
            result["wall_clock_ms"] = float(m.group(1))
    return result


# ──────────────────────────────────────────────────────────
# Experiment runner
# ──────────────────────────────────────────────────────────

def compute_timeout(nn: int, bandwidth: str) -> int:
    """
    Compute adaptive timeout based on expected communication size.
    Communication scales roughly as 4x per 2 nn steps (from ~6 MB at nn=14).
    At nn=22: ~1455 MB. On 10Mbps (1.25 MB/s): ~1164s transmission + overhead.
    Returns timeout in seconds with 3x safety margin.
    """
    # Estimated comm in MB at each nn, based on observed scaling
    comm_est_mb = 6.25 * (4 ** ((nn - 14) / 2.0))
    # Bandwidth in MB/s
    bw_str = bandwidth.lower()
    if "gbit" in bw_str:
        bw_mbps = float(bw_str.replace("gbit", "")) * 1000
    elif "mbit" in bw_str:
        bw_mbps = float(bw_str.replace("mbit", ""))
    else:
        bw_mbps = 10000  # default LAN
    bw_mbps = bw_mbps / 8.0  # MB/s

    # Transmission time + processing overhead + base constant
    xmit_sec = comm_est_mb / bw_mbps if bw_mbps > 0 else 0
    # Safety margin: 3x for WAN (processing + variance), min 600s
    timeout = max(int(xmit_sec * 3 + 300), 600)
    return timeout


def run_single(nn: int, scheme_name: str, net_name: str, bandwidth: str,
               delay: str, extra_tc: str, protocol: str) -> dict:
    """
    Run a single protocol execution in a network namespace and return parsed results.
    protocol is either 'rpmt' or 'psu'.
    """
    scheme = SCHEMES[scheme_name]
    bin_path = scheme["dir"] / (scheme["rpmt_bin"] if protocol == "rpmt" else scheme["psu_bin"])
    extra_args = scheme["rpmt_extra_args"] if protocol == "rpmt" else scheme["psu_extra_args"]

    ns_name = f"exp_{scheme_name}_{protocol}_{net_name}_{nn}"
    timeout = compute_timeout(nn, bandwidth)

    # Clean up any leftover namespace
    subprocess.run(["ip", "netns", "del", ns_name], capture_output=True)

    # Create namespace and set up tc
    subprocess.run(["ip", "netns", "add", ns_name], check=True)
    subprocess.run(["ip", "netns", "exec", ns_name, "ip", "link", "set", "dev", "lo", "up"], check=True)

    tc_cmd = ["tc", "qdisc", "add", "dev", "lo", "root", "netem", "delay", delay, "rate", bandwidth]
    if extra_tc:
        tc_cmd.extend(extra_tc.split())
    subprocess.run(["ip", "netns", "exec", ns_name] + tc_cmd, check=True)

    p0 = None
    p1 = None
    try:
        cmd_p0 = [str(bin_path), "-r", "0", "-nn", str(nn)] + extra_args
        cmd_p1 = [str(bin_path), "-r", "1", "-nn", str(nn)] + extra_args

        p0 = subprocess.Popen(
            ["ip", "netns", "exec", ns_name] + cmd_p0,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )
        time.sleep(2)

        p1 = subprocess.Popen(
            ["ip", "netns", "exec", ns_name] + cmd_p1,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
        )

        out1, _ = p1.communicate(timeout=timeout)
        p0.communicate(timeout=60)

        parsed = parse_output(out1)
        if not parsed:
            print(f"  [WARN] Failed to parse output.", file=sys.stderr)
        return parsed

    except subprocess.TimeoutExpired:
        print(f"  [TIMEOUT] after {timeout}s", file=sys.stderr)
        # Kill processes
        for p in (p0, p1):
            if p and p.poll() is None:
                p.kill()
                try:
                    p.communicate(timeout=5)
                except Exception:
                    pass
        return {"error": "TIMEOUT"}

    except Exception as e:
        print(f"  [ERROR] {e}", file=sys.stderr)
        for p in (p0, p1):
            if p and p.poll() is None:
                p.kill()
                try:
                    p.communicate(timeout=5)
                except Exception:
                    pass
        return {"error": str(e)}

    finally:
        subprocess.run(["ip", "netns", "del", ns_name], capture_output=True)


# ──────────────────────────────────────────────────────────
# Experiment matrix runner
# ──────────────────────────────────────────────────────────

def load_completed(csv_path: Path) -> set:
    """Load already-completed experiment keys from existing CSV."""
    if not csv_path.exists():
        return set()
    completed = set()
    with open(csv_path, "r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            key = (row["scheme"], row["network"], int(row["nn"]))
            completed.add(key)
    return completed


def make_result_row(scheme_name, net_name, nn, parsed):
    """Build a result row dict from parsed output, handling error cases."""
    if parsed is None:
        return {"scheme": scheme_name, "network": net_name, "nn": nn,
                "total_time_ms": "PARSE_ERROR", "total_comm_mb": "PARSE_ERROR",
                "wall_clock_ms": "PARSE_ERROR"}
    if "error" in parsed:
        return {"scheme": scheme_name, "network": net_name, "nn": nn,
                "total_time_ms": parsed["error"], "total_comm_mb": parsed["error"],
                "wall_clock_ms": parsed["error"]}
    return {"scheme": scheme_name, "network": net_name, "nn": nn,
            "total_time_ms": parsed.get("total_time_ms", ""),
            "total_comm_mb": parsed.get("total_comm_mb", ""),
            "wall_clock_ms": parsed.get("wall_clock_ms", "")}


def run_fig22(output_dir: Path, resume: bool = False):
    """Run all mqRPMT experiments (fig22)."""
    csv_path = output_dir / "fig22_mqRPMT.csv"
    completed = load_completed(csv_path) if resume else set()
    rows = []

    total = len(SCHEMES) * len(NET_CONFIGS) * len(NN_LIST)
    skipped = 0
    count = 0

    for scheme_name in SCHEMES:
        for net_name, bw, delay, extra_tc in NET_CONFIGS:
            for nn in NN_LIST:
                count += 1
                key = (scheme_name, net_name, nn)
                label = f"[{count}/{total}] mqRPMT {scheme_name} {net_name} nn=2^{nn}"

                if key in completed:
                    print(f"{label} ... SKIP (already completed)")
                    skipped += 1
                    continue

                print(f"{label} ... ", end="", flush=True)
                parsed = run_single(nn, scheme_name, net_name, bw, delay, extra_tc, "rpmt")
                row = make_result_row(scheme_name, net_name, nn, parsed)
                rows.append(row)

                if "error" in (parsed or {}):
                    print(parsed["error"])
                else:
                    print(f"OK ({row['total_time_ms']} ms, {row['total_comm_mb']} MB)")

    if skipped:
        print(f"[INFO] Skipped {skipped} already-completed experiment(s)")

    if rows:
        write_csv(csv_path, rows, append=(resume and completed))
    print(f"[INFO] fig22 results saved to: {csv_path}")
    return rows


def run_fig24(output_dir: Path, resume: bool = False):
    """Run all PSU experiments (fig24)."""
    csv_path = output_dir / "fig24_PSU.csv"
    completed = load_completed(csv_path) if resume else set()
    rows = []

    total = len(SCHEMES) * len(NET_CONFIGS) * len(NN_LIST)
    skipped = 0
    count = 0

    for scheme_name in SCHEMES:
        for net_name, bw, delay, extra_tc in NET_CONFIGS:
            for nn in NN_LIST:
                count += 1
                key = (scheme_name, net_name, nn)
                label = f"[{count}/{total}] PSU {scheme_name} {net_name} nn=2^{nn}"

                if key in completed:
                    print(f"{label} ... SKIP (already completed)")
                    skipped += 1
                    continue

                print(f"{label} ... ", end="", flush=True)
                parsed = run_single(nn, scheme_name, net_name, bw, delay, extra_tc, "psu")
                row = make_result_row(scheme_name, net_name, nn, parsed)
                rows.append(row)

                if "error" in (parsed or {}):
                    print(parsed["error"])
                else:
                    print(f"OK ({row['total_time_ms']} ms, {row['total_comm_mb']} MB)")

    if skipped:
        print(f"[INFO] Skipped {skipped} already-completed experiment(s)")

    if rows:
        write_csv(csv_path, rows, append=(resume and completed))
    print(f"[INFO] fig24 results saved to: {csv_path}")
    return rows


# ──────────────────────────────────────────────────────────
# Output helpers
# ──────────────────────────────────────────────────────────

def write_csv(path: Path, rows: list, append: bool = False):
    """Write results to CSV file."""
    fieldnames = ["scheme", "network", "nn", "total_time_ms", "total_comm_mb", "wall_clock_ms"]
    mode = "a" if append else "w"
    write_header = not append or not path.exists()
    with open(path, mode, newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()
        writer.writerows(rows)


def print_summary(rows: list, label: str):
    """Print a formatted summary table of results."""
    print(f"\n{'='*80}")
    print(f"  {label} Summary")
    print(f"{'='*80}")
    header = f"{'Scheme':<6} {'Network':<16} {'nn':>4}  {'Time(ms)':>12}  {'Comm(MB)':>10}"
    print(header)
    print("-" * len(header))
    for r in rows:
        print(f"{r['scheme']:<6} {r['network']:<16} {r['nn']:>4}  "
              f"{str(r['total_time_ms']):>12}  {str(r['total_comm_mb']):>10}")
    print(f"{'='*80}")


# ──────────────────────────────────────────────────────────
# Main
# ──────────────────────────────────────────────────────────

def main():
    global NN_LIST, NET_CONFIGS

    parser = argparse.ArgumentParser(
        description="Run fig22 (mqRPMT) and fig24 (PSU) experiments for ePSU."
    )
    parser.add_argument("--fig22", action="store_true", default=False,
                        help="Run mqRPMT experiments (fig22)")
    parser.add_argument("--fig24", action="store_true", default=False,
                        help="Run PSU experiments (fig24)")
    parser.add_argument("--output-dir", type=str, default="./results",
                        help="Directory for output CSV files (default: ./results)")
    parser.add_argument("--small-test", action="store_true", default=False,
                        help="Run a small test with nn=14 only, LAN only, to verify setup")
    parser.add_argument("--resume", action="store_true", default=False,
                        help="Resume from existing CSV, skipping already-completed experiments")
    args = parser.parse_args()

    # If neither specified, run both
    run_fig22_flag = args.fig22 or (not args.fig22 and not args.fig24)
    run_fig24_flag = args.fig24 or (not args.fig22 and not args.fig24)

    # Check sudo
    if os.geteuid() != 0:
        print("[ERR] This script requires sudo for network namespace operations.")
        print("      Please run: sudo python3 run_experiments.py [args]")
        sys.exit(1)

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    if args.small_test:
        NN_LIST = [14]
        NET_CONFIGS = [NET_CONFIGS[0]]

    print("=" * 60)
    print("  ePSU Experiment Runner")
    print("=" * 60)
    print(f"  Output directory: {output_dir}")
    print(f"  Experiments:")
    if run_fig22_flag:
        print(f"    fig22 (mqRPMT): {len(SCHEMES)} schemes x {len(NET_CONFIGS)} nets x {len(NN_LIST)} nns = {len(SCHEMES)*len(NET_CONFIGS)*len(NN_LIST)} runs")
    if run_fig24_flag:
        print(f"    fig24 (PSU):    {len(SCHEMES)} schemes x {len(NET_CONFIGS)} nets x {len(NN_LIST)} nns = {len(SCHEMES)*len(NET_CONFIGS)*len(NN_LIST)} runs")
    if args.small_test:
        print("  [INFO] Small test mode: nn=14, LAN only")
    print("=" * 60)
    print()

    all_rows = []

    if run_fig22_flag:
        print("[INFO] Starting fig22 (mqRPMT) experiments...")
        rows = run_fig22(output_dir, resume=args.resume)
        all_rows.extend(rows)
        print_summary(rows, "fig22 (mqRPMT)")

    if run_fig24_flag:
        print("[INFO] Starting fig24 (PSU) experiments...")
        rows = run_fig24(output_dir, resume=args.resume)
        all_rows.extend(rows)
        print_summary(rows, "fig24 (PSU)")

    print(f"\n[DONE] All experiments complete. Results in: {output_dir}")


if __name__ == "__main__":
    main()
