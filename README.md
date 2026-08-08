# Enhanced Private Set Union from Secret-Shared Private Membership Test

## Introduction

This repository contains the artifact for evaluating two implementations of enhanced Private Set Union (ePSU). The artifact focuses on reproducing the timing and communication results for the protocol and its main building blocks.

The repository is organized into two largely independent implementations:

1. `ePSU_fast`: the fast ePSU implementation based on `ssPMT`.
2. `ePSU_low`: the low-communication ePSU implementation based on `psPMT`.

Both implementations produce standardized logs. Each measured module reports running time in milliseconds and communication in MB, followed by total module cost and wall-clock time.

## Information for Artifact Reviewers

The main experimental claims supported by this artifact are:

| Claim | Supported by | Scripts |
| --- | --- | --- |
| Full ePSU performance is reproducible. | End-to-end benchmarks for `ePSU_fast` and `ePSU_low`. | `ePSU_batch_bench.sh` |
| PMT component performance is reproducible. | `ssPMT` for the fast variant and `psPMT` for the low-communication variant. | `ssPMT_batch_bench.sh`, `psPMT_batch_bench.sh` |
| RPMT component performance is reproducible. | RPMT benchmarks in both implementations. | `RPMT_batch_bench.sh` |

The batch scripts run both protocol parties, emulate the configured network settings, and write logs into timestamped directories such as `logs_ePSU_YYYYMMDD_HHMMSS/`.

## Repository Layout

| Path | Description |
| --- | --- |
| `ePSU_fast/` | Fast implementation, including `ePSU`, `ssPMT`, `RPMT`, `ssOTd`, `SoOPPRF`, and `PSU` test executables. |
| `ePSU_low/` | Low-communication implementation, including `ePSU`, `psPMT`, `RPMT`, `ssOTd`, and `PSU` test executables. |
| `Dockerfile` | Container recipe for the artifact environment. |
| `README.md` | This artifact overview and reproduction guide. |

## Supported Environments

The code is intended for Linux environments with CMake, a C++ compiler with C++20 support, Python 3, Git, OpenMP, and standard networking tools.

The batch benchmark scripts require:

- `sudo` (for simulated network)
- Linux network namespaces via `ip netns`
- traffic control via `tc netem`

The setup scripts download and build third-party dependencies under each implementation's `thirdparty/` directory.

## Quick Start

### Native Build

Build the fast implementation:

```bash
cd ePSU_fast
bash setup.sh
```

Build the low-communication implementation:

```bash
cd ePSU_low
bash setup.sh
```

Run a small local end-to-end benchmark after building:

```bash
cd ePSU_fast
bash ePSU_bench.sh 14
```

```bash
cd ePSU_low
bash ePSU_bench.sh 14
```

The argument `14` means each party uses a set of size `2^14`.

### Docker Build (Recommended)

The repository includes a Dockerfile for a containerized artifact environment. Building the image takes about 10 minutes on a typical workstation and builds both `ePSU_fast` and `ePSU_low` automatically.

Build the image from the repository root:

```bash
docker build -t epsu-sspmt .
```

Start the container with `--privileged`, which is required for the benchmark scripts that use Linux network namespaces and `tc netem`:

```bash
docker run --rm -it --privileged epsu-sspmt bash
```

## Run Time of Experiments

The full batch scripts evaluate several set sizes and three network settings:

| Network | Rate | Delay |
| --- | --- | --- |
| `LAN` | `10gbit` | `100us` |
| `WAN_100Mbps` | `100mbit` | `40ms` |
| `WAN_10Mbps` | `10mbit` | `40ms` |

The default set sizes are `2^14`, `2^16`, `2^18`, `2^20`, and `2^22`. Runtime depends heavily on the machine and the network emulation configuration. For a quick smoke test, edit `NN_LIST` in the corresponding batch script to a smaller subset, for example:

```bash
NN_LIST=(14)
```

## Overview of Tables and Figures

The following roadmap links the artifact scripts to the paper results referenced by this implementation:

| Paper result | Experiment | Fast script | Low script |
| --- | --- | --- | --- |
| Figure 24, Table 2, Table 3 | Full ePSU | `ePSU_fast/ePSU_batch_bench.sh` | `ePSU_low/ePSU_batch_bench.sh` |
| Figure 21 | PMT component | `ePSU_fast/ssPMT_batch_bench.sh` | `ePSU_low/psPMT_batch_bench.sh` |
| Figure 22 | RPMT component | `ePSU_fast/RPMT_batch_bench.sh` | `ePSU_low/RPMT_batch_bench.sh` |

> For automated collection of Figure 22 and Figure 24 data into structured CSV files, use the root-level `run_experiments.py` script. See the [Automated Experiment Collection](#automated-experiment-collection-run_experimentspy) section below.

### Figure 22 — RPMT (multi-query Relaxed PMT) Performance

Figure 22 evaluates the performance of the **RPMT** (Relaxed Private Membership Test) protocol, which is the core building block for the enhanced PSU construction. This experiment measures:

- **Protocol**: RPMT (Fast variant uses `SoOPPRF` + `PEQT`; Low-communication variant uses `OPPRF` + `ssPEQT` + `OPermute`)
- **Schemes**: Both the **fast** (`ePSU_fast`) and **low-communication** (`ePSU_low`) implementations
- **Network conditions**: LAN (10 Gbps, 100 us delay), WAN_100Mbps (100 Mbps, 40 ms delay), WAN_10Mbps (10 Mbps, 40 ms delay)
- **Set sizes**: 2^14, 2^16, 2^18, 2^20, 2^22
- **Metrics collected**: Total protocol computation time (ms) and communication overhead (MB)

### Figure 24 — Full ePSU (Private Set Union) Performance

Figure 24 evaluates the end-to-end performance of the **ePSU** (enhanced Private Set Union) protocol, which constructs the full PSU from the RPMT building block plus an OT-based set reconciliation step. This experiment measures:

- **Protocol**: PSU (RPMT + OT-based union recovery)
- **Schemes**: Both the **fast** (`ePSU_fast`) and **low-communication** (`ePSU_low`) implementations
- **Network conditions**: Same three network settings as Figure 22
- **Set sizes**: 2^14, 2^16, 2^18, 2^20, 2^22
- **Metrics collected**: Total protocol computation time (ms) and communication overhead (MB)

## Reproducing the Results

Run the full ePSU experiments:

```bash
cd ePSU_fast
sudo bash ePSU_batch_bench.sh
```

```bash
cd ePSU_low
sudo bash ePSU_batch_bench.sh
```

Run the PMT component experiments:

```bash
cd ePSU_fast
sudo bash ssPMT_batch_bench.sh
```

```bash
cd ePSU_low
sudo bash psPMT_batch_bench.sh
```

Run the RPMT component experiments:

```bash
cd ePSU_fast
sudo bash RPMT_batch_bench.sh
```

```bash
cd ePSU_low
sudo bash RPMT_batch_bench.sh
```

Each batch script creates a fresh log directory in the implementation folder. The log files are grouped by network setting, for example:

```text
logs_ePSU_YYYYMMDD_HHMMSS/
  LAN.log
  WAN_100Mbps.log
  WAN_10Mbps.log
```

## Automated Experiment Collection (`run_experiments.py`)

For a streamlined workflow, the root-level `run_experiments.py` script automates running **Figure 22** (RPMT) and **Figure 24** (PSU) experiments and collecting the results into structured CSV files. It runs both fast and low-communication schemes across all network conditions and set sizes, parses the standardized log output, and produces CSV tables ready for plotting.

### Prerequisites

Build both implementations first:

```bash
cd ePSU_fast && bash setup.sh && cd ..
cd ePSU_low && bash setup.sh && cd ..
```

The script requires `sudo` for network namespace and `tc netem` operations.

### Quick Smoke Test

Verify the setup with a minimal run (nn=14, LAN only, both protocols):

```bash
sudo python3 run_experiments.py --small-test --output-dir ./results_test
```

### Reproducing Figure 22 (RPMT)

```bash
sudo python3 run_experiments.py --fig22 --output-dir ./results
```

This runs 30 experiments (2 schemes x 3 networks x 5 set sizes) and produces `./results/fig22_mqRPMT.csv`.

### Reproducing Figure 24 (PSU)

```bash
sudo python3 run_experiments.py --fig24 --output-dir ./results
```

This runs 30 experiments and produces `./results/fig24_PSU.csv`.

### Running Both Figures at Once (60 experiments total)

```bash
sudo python3 run_experiments.py --output-dir ./results
```

### Resuming an Interrupted Run

If the script is interrupted (e.g., due to system reboot or a transient error), use `--resume` to skip experiments already recorded in the output CSV:

```bash
sudo python3 run_experiments.py --fig22 --output-dir ./results --resume
```

### Expected Runtime

WAN experiments with large set sizes (nn=22) dominate the total runtime. On a typical workstation:

| Network | nn=14 | nn=16 | nn=18 | nn=20 | nn=22 |
| --- | --- | --- | --- | --- | --- |
| LAN (10 Gbps) | seconds | seconds | seconds | tens of seconds | ~2 minutes |
| WAN_100Mbps (100 Mbps) | seconds | seconds | tens of seconds | ~1 minute | ~3-5 minutes |
| WAN_10Mbps (10 Mbps) | seconds | tens of seconds | ~1 minute | ~5 minutes | ~20-30 minutes |

Total wall-clock time for all 60 experiments is approximately 2-4 hours, depending on hardware. Adjust `NN_LIST` in the script to reduce the set sizes for a faster run.

### CSV Output Format

The generated CSV files have the following columns:

| Column | Description |
| --- | --- |
| `scheme` | `fast` (ePSU_fast) or `low` (ePSU_low) |
| `network` | `LAN`, `WAN_100Mbps`, or `WAN_10Mbps` |
| `nn` | Set size exponent: 14, 16, 18, 20, or 22 |
| `total_time_ms` | Sum of all module times reported by the protocol (ms) |
| `total_comm_mb` | Total communication across all modules (MB) |
| `wall_clock_ms` | End-to-end wall-clock duration (ms) |

Example row:

```csv
scheme,network,nn,total_time_ms,total_comm_mb,wall_clock_ms
fast,LAN,14,623.095,6.249,624.012
low,WAN_10Mbps,22,1217.357,2.307,1225.924
```

## Output Format

The standardized log format is:

```text
========================================================
  Protocol: ePSU-Fast | Role: Party 1 | Set Size: 2^14 = 16384
--------------------------------------------------------
  [SoOPPRF]        123.456 ms |     12.345 MB
  [ssPEQT]         456.789 ms |     34.567 MB
  [ssOTd]          234.567 ms |     56.789 MB
--------------------------------------------------------
  [TOTAL]          814.812 ms |    103.701 MB
  [WALL CLOCK]     850.123 ms
========================================================
```

The fields are:

| Field | Meaning |
| --- | --- |
| Module name | Protocol sub-module being measured. |
| Time | Elapsed time for that module in milliseconds. |
| Communication | Incremental communication for that module in MB. |
| `TOTAL` | Sum of recorded module times and communication costs. |
| `WALL CLOCK` | End-to-end wall-clock duration for the measured party. |

## Manual Execution

The protocols are two-party protocols. The helper scripts start both parties automatically, but executables can also be run manually.

Example for `ePSU_fast`:

```bash
cd ePSU_fast
./build/ePSU -r 0 -nn 14
```

In a second terminal:

```bash
cd ePSU_fast
./build/ePSU -r 1 -nn 14
```

The common parameters are:

| Parameter | Description | Example |
| --- | --- | --- |
| `-r` | Party role, either `0` or `1`. | `-r 0` |
| `-nn` | Set size exponent. The set size is `2^nn`. | `-nn 14` |

## Included Results

This repository may contain generated `logs_*` directories from prior runs. These logs are examples of the artifact output format and may differ from newly generated results because performance depends on hardware, CPU load, OS scheduling, compiler version, and network emulation behavior.

## Reviewer Roadmap

For a short review pass:

1. Build natively with `setup.sh`, or preferably build the Docker image with `docker build -t epsu-sspmt .`.
2. Run `bash ePSU_bench.sh 14` in `ePSU_fast` after building `ePSU_fast`.
3. Run `bash ePSU_bench.sh 14` in `ePSU_low` after building `ePSU_low`.
4. Inspect the standardized module-level logs printed by party 1.

For a fuller evaluation:

1. Run the fast and low full ePSU batch scripts.
2. Run the PMT batch scripts.
3. Run the RPMT batch scripts.
4. Compare the generated `LAN.log`, `WAN_100Mbps.log`, and `WAN_10Mbps.log` files against the corresponding paper tables and figures.

For automated reproduction of Figure 22 and Figure 24 with structured CSV output:

1. Build both implementations with `setup.sh`.
2. Run `sudo python3 run_experiments.py --small-test --output-dir ./results_test` to verify the setup.
3. Run `sudo python3 run_experiments.py --output-dir ./results` to collect all data.
4. Use the generated `results/fig22_mqRPMT.csv` and `results/fig24_PSU.csv` for plotting.

## Notes

- Run `setup.sh` before running any benchmark script.
- Batch scripts require `sudo` because they configure Linux network namespaces and `tc netem`.
- Existing logs are not overwritten; each batch run creates a new timestamped log directory.
- Rebuild an implementation after editing headers such as `StdLog.h`, otherwise existing binaries may still use the old logging format.
