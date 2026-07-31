# 🐜 Anthill

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

## 📜 License

This project is open-source and available under the [MIT License](./LICENSE).
