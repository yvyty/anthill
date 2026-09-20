# 🐜 Anthill

[![Build and Release](https://github.com/yvyty/anthill/actions/workflows/release.yml/badge.svg)](https://github.com/yvyty/anthill/actions/workflows/release.yml) [![GitHub release](https://img.shields.io/github/v/release/yvyty/anthill.svg?color=success)](https://github.com/yvyty/anthill/releases) [![Language](https://img.shields.io/badge/Language-C11-%234c1.svg)](https://github.com/yvyty/anthill) [![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

**Anthill** is a lightweight, high-performance CLI tool written in native C that rapidly enumerates all bindable (available) ports on a host machine. 

Instead of sequential, slow network polling, Anthill acts as a command center, deploying lightweight scout "ants" (workers) to map the terrain of your system's port availability across Linux, macOS, and Windows. 

## ✨ Features
* **Cross-Platform:** Native support for POSIX sockets (Linux/macOS) and Winsock2 (Windows).
* **High Performance:** Written in C with strict optimization flags to keep overhead practically non-existent.
* **Zero Dependencies:** Relies only on standard system libraries and CMake for building.
* **Developer-Friendly:** Cleanly structured for immediate integration into larger systems programming or homelab projects.

## 🚀 Getting Started

### Prerequisites
* A C11-compatible compiler (GCC, Clang, or MSVC)
* [CMake](https://cmake.org/) (Version 3.10 or higher)

### Build Instructions

To spawn your own Anthill, clone the repository and compile via CMake:

```bash
git clone https://github.com/yvyty/anthill.git
cd anthill

# Create the build directory
mkdir build && cd build

# Generate build files and compile
cmake ..
make

```

*(Note for Windows users: Use `cmake --build . --config Release` instead of `make`)*

### Usage

Simply run the executable. The colony will deploy and return a status report of available ports on `localhost`.

```bash
./anthill
```

Narrow the sweep with `-r`, and pick an output shape when a script is consuming
the result:

```bash
./anthill -r 8080-8085    # scan a small range
./anthill -c              # count only
./anthill -j              # JSON
./anthill -o available.txt  # write the report to a file
```

**Example Output** (`./anthill -r 8080-8085`):

```text
Spawning Anthill...
[Anthill] POSIX network ready. Ants are ready.
Deploying 6 ant squadrons to check ports 8080-8085...
    -> Squadron 01 completed (Ports 8080 to 8080).
    -> Squadron 02 completed (Ports 8081 to 8081).
    -> Squadron 03 completed (Ports 8082 to 8082).
    -> Squadron 04 completed (Ports 8083 to 8083).
    -> Squadron 05 completed (Ports 8084 to 8084).
    -> Squadron 06 completed (Ports 8085 to 8085).

Anthill dormant. All ants returned.
Available ports (6):
    8080
    8081
    8082
    8083
    8084
    8085
Total available ports: 6
```

Squadron progress and the "All ants returned" banner go to **stderr**, and each
squadron prints when it finishes, so those lines can appear out of numeric
order. The port listing and total go to **stdout** (or to `-o PATH`), so piping
or redirecting the report never picks up the chatter.

### Options

| Flag | Argument | Description |
| --- | --- | --- |
| `-r` | `START-END` | Port range to scan (default: `1-65535`). |
| `-c` | | Print only the total count of available ports. |
| `-j` | | Emit the result as JSON (`start_port`, `end_port`, `available_ports`). |
| `--list` | | List every available port (default for human output). |
| `--no-list` | | Suppress the per-port listing. |
| `-o` | `PATH` | Write results to `PATH` instead of stdout. |
| `-x` | `PORTS` | Exclude ports, e.g. `-x 22,80,8000-9000`. |
| `-i` | `PORTS` | Only check these ports, e.g. `-i 80,443`. |
| `--host` | `HOST` | Scan `HOST` instead of localhost (default: `127.0.0.1`). |
| `--timeout` | `MS` | Connect timeout in milliseconds for remote hosts (default: `200`, max: `60000`). |
| `-u`, `--udp` | | Check UDP ports instead of TCP. |
| `-t`, `--threads` | `N` | Squadron count (default: `16`, max: `256`; larger values are clamped). |
| `--progress` | | Print periodic progress to stderr. |
| `-h`, `--help` | | Show the help message. |

## 🎯 Scanning remote hosts and UDP

By default Anthill answers a **local** question: "can I bind this port?" A port
is reported available when the bind succeeds, i.e. nothing is using it yet.

Pass `--host` to point the scan at another machine. Remote scans answer a
different question: "is something serving this port?" Anthill uses a
non-blocking `connect()` bounded by `--timeout` (default 200 ms), so filtered
ports fail fast instead of waiting on the OS default. The two modes are not
interchangeable: a port can be free to bind locally yet unreachable remotely,
and vice versa.

```bash
./anthill --host 192.168.1.10 -r 80-90 --timeout 500
```

`-u` / `--udp` switches to UDP. UDP has no handshake, so in v1 "available"
means only that the **local** box can bind the port (`SOCK_DGRAM`). Remote UDP
reachability cannot be established reliably, so `-u` is rejected together with
a non-loopback `--host`. A datagram can be sent and silently dropped, which is
indistinguishable from a closed port without ICMP feedback.

## ⚠️ Limitations

- **Privileged ports.** On POSIX, binding a port below 1024 requires root. When
  run without root, Anthill reports those ports as **unavailable** even if
  nothing is listening on them, so a low port can be a false negative rather
  than a port that is genuinely in use. Run under `sudo` if you need an accurate
  answer for the range `1-1023`.
- **Local vs. remote, and UDP.** A local bind answers "is this port free?"
  while a remote connect answers "is something serving this port?", and UDP is
  local-only in v1. See [Scanning remote hosts and UDP](#-scanning-remote-hosts-and-udp)
  above before mixing those modes.

## 📜 License

This project is open-source and available under the [MIT License](./LICENSE).
