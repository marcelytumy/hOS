## hOS

Minimal hobby operating system that boots with Limine and is written in C++.

### Quick start

1) Fetch kernel-side headers and runtime:
```bash
./kernel/get-deps
```
2) Build and run in QEMU (x86_64 by default):
```bash
make TOOLCHAIN=llvm run
```

### Prerequisites

- **Build tools**: GNU `make`, Clang/LLVM (recommended) or GCC, `git`, `curl`.
- **ISO/HDD tooling**: `xorriso` (ISO), `sgdisk` and `mtools` (HDD).
- **Emulator**: `qemu-system-<arch>` (OVMF firmware is downloaded automatically).

### Build and run

- **Select toolchain**:
  - `TOOLCHAIN=llvm` to use Clang/LLVM, or set a prefix via `TOOLCHAIN_PREFIX` (e.g. `x86_64-elf-`).
- **Select architecture**:
  - `ARCH=x86_64` (default), `aarch64`, `riscv64`, `loongarch64`.
- **Targets**:
  - `make all` → build kernel and ISO
  - `make run` → build and boot ISO in QEMU
  - `make all-hdd` → build kernel and raw HDD image
  - `make run-hdd` or `make dev-run` → build and boot HDD image in QEMU

Examples:
```bash
make TOOLCHAIN=llvm ARCH=x86_64 run
make TOOLCHAIN=llvm ARCH=aarch64 run
```

### Optional root filesystem

If a `rootfs.img` file is present in the project root, it is automatically bundled as a boot module into the ISO/HDD images.

### Project layout

- **kernel/**: core kernel sources, basic filesystems, low-level platform glue.
- **ui/**: minimal immediate-mode UI and example applications.
- **input/**: basic input handling used by the UI.
- **limine/**, **ovmf/**: fetched at build time by the makefiles.

### Roadmap (high-level)

- **Filesystem**: richer EXT4 support, path resolution, symlinks, and robust I/O.
- **Process model**: tasks/threads, scheduling, and messaging primitives.
- **Memory**: paging, virtual memory management, and safer allocators.
- **Drivers**: storage, timers, input beyond legacy devices, and basic PCI.
- **Graphics/UI**: damage tracking, text input, widgets, and window management QoL.

### License

See `LICENSE` for licensing information.
