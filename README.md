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

**Example Output:**

```text
Spawning Anthill...
[Anthill] POSIX network ready. Ants are ready.
Scout ants deploying to ports 1 through 65535...

Port 80: [AVAILABLE]
Port 81: [AVAILABLE]
...
Anthill dormant. All ants returned.
Total available ports on localhost: 65412

```

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

## 📜 License

This project is open-source and available under the [MIT License](./LICENSE).
