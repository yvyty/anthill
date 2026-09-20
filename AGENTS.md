# AGENTS.md

Guidance for AI agents working in this repository.

## Project Overview

**Anthill** is a cross-platform CLI tool written in C11 that enumerates all
bindable (available) TCP ports on localhost. It probes a port by attempting to
`bind()` a socket to it, then closes the socket. The scan is parallelized
across worker threads ("squadrons").

- Zero external dependencies: only system socket libraries and pthreads/Win32 threads.
- Platforms: Linux, macOS, Windows (Winsock2).
- Repo intentionally minimal: two source files total.

## Repository Layout

```
src/main.c           - entire application: CLI parsing, worker threads, main()
include/net_utils.h  - cross-platform abstraction layer (sockets, threads, mutex)
CMakeLists.txt       - the only build definition
.github/workflows/release.yml - CI: release-time matrix build for linux/macos/windows
build/               - local CMake output (git-ignored)
```

There is no separate `.c` file for `net_utils.h`; all platform helpers in it
are `static inline` and live entirely in the header.

## Build & Run

```bash
# Configure and build (matches CI)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run
./build/anthill                 # scan ports 1-65535
./build/anthill -r 8000-9000    # custom range
./build/anthill -c              # count only
./build/anthill -j              # JSON output
```

CI artifact path gotcha: MSVC places the binary in `build/Release/anthill.exe`
while POSIX generators output `build/anthill`. `release.yml` handles this when
packaging; if you change the target name, update that workflow.

## Tests

There is no test suite, test framework, or CI check on push (CI only runs on
GitHub release publish). Verify changes by building with warnings enabled
(`-Wall -Wextra` / MSVC `/W4` are already set in CMakeLists.txt) and running
the binary against a small port range, e.g. `./build/anthill -r 8080-8085`.
Keep the build warning-free.

## Architecture & Data Flow

1. `main()` parses args into `app_config_t` (`-r`, `-c`, `-j`, `-h`).
2. `init_network_workers()` runs once (real `WSAStartup` on Windows, a no-op
   banner print on POSIX).
3. The port range is divided evenly across up to `MAX_THREADS` (16) workers;
   the last worker absorbs any remainder by running to `end_port` explicitly.
   If the range is smaller than `MAX_THREADS`, one thread per port is used.
4. Each worker (`ant_worker`) iterates its sub-range, calls
   `check_port_availability()` (socket + bind to 127.0.0.1), accumulates a
   **local** count, then merges it into the shared total under a mutex exactly
   once before exiting. There is no per-port locking.
5. `main()` joins all workers, prints the result in the selected format, and
   calls `cleanup_network_workers()`.

## Conventions & Gotchas

- **Portability is the central concern.** Never call pthread, POSIX socket,
  or Win32 APIs directly in `main.c`. Add to the abstraction surface in
  `net_utils.h` instead (typed `anthill_*` wrappers for threads/mutexes,
  `socket_fd_t`, `CLOSE_SOCKET`, `VALID_SOCKET` macros). The file is split
  into a single `#ifdef _WIN32 / #else` block; keep both branches in sync when
  adding wrappers.
- **Thread function signatures differ per platform.** Always declare workers
  with the `ANTHILL_THREAD_FUNC` macro and return with
  `(ANTHILL_THREAD_RETURN)0`. Hardcoding `void *` breaks the Windows build.
- **Windows threads must be created via `_beginthreadex`** (wrapped in
  `anthill_thread_create`), not `CreateThread`; keep it that way so CRT
  functions remain thread-safe.
- **Windows builds define `_CRT_SECURE_NO_WARNINGS`** and use `sscanf`; do not
  "modernize" to `sscanf_s` (not portable) and do not remove the define.
- Style: 4-space indent, Allman braces in control flow are rare; functions use
  K&R-style opening brace on next line for definitions. `printf`/`fprintf`
  calls with long strings are split one argument per line (see existing code).
  Types and functions use `_t` / snake_case suffixes (`worker_args_t`,
  `check_port_availability`).
- The "anthill" vocabulary (squadrons, ants, colony) in user-facing strings
  and identifiers is intentional branding; preserve it when editing output
  messages.
- `CMAKE_EXPORT_COMPILE_COMMANDS` is ON; `build/compile_commands.json` is
  generated for clangd but is git-ignored (as are `build/` and the `anthill`
  binary itself).
- **Ports below 1024 are privileged on POSIX.** `bind()` there requires root, so
  an unprivileged run reports those ports as unavailable even when nothing is
  listening. Treat that as a false negative, not a busy port, when changing scan
  logic or docs.

## Releases

Releases are cut by publishing a GitHub release, which triggers
`.github/workflows/release.yml` to build on all three OSes and upload
binaries. There is no version constant in the source; versioning lives only
in git tags/release names.
