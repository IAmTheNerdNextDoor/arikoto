# Arikoto (ありこと)
[![CodeQL Runner](https://github.com/AFellowSpeedrunner/arikoto/actions/workflows/codeql.yml/badge.svg)](https://github.com/AFellowSpeedrunner/arikoto/actions/workflows/codeql.yml) ![loc](https://tokei.rs/b1/github/IAmTheNerdNextDoor/arikoto) [![justforfunnoreally.dev badge](https://img.shields.io/badge/justforfunnoreally-dev-9ff)](https://justforfunnoreally.dev)

Arikoto is my personal Operating System development project that is inspired by and is aiming to have a similar nature to [ToaruOS](https://github.com/klange/toaruos).

## Features

- Written in C/inline and normal Assembly
- 64-bit
- UEFI + BIOS support
- Limine boot protocol
- Physical Memory
- Virtual Memory
- VFS/External CPIO (newc) Ramdisk
- CMake Build System
- Keyboard
- Shell
- GDT
- TSS
- IDT
- PIC
- Timer
- ISR
- IRQ (Partial)
- Paging
- Kernel Heap
- Preemptive Scheduling

## Building

### Linux

Run `mkdir build && cd build` in the Arikoto root folder. Type `cmake ..`. The project will be configured automatically.

From here, you can type `make`, `make run`, `make run-kvm`, `make run-uefi` and (my favourite) `make run-uefi-kvm`.

Running `make clean` wipes built binaries and arikoto.iso if an image was built. Running `make distclean` wipes the `limine/`, `ovmf/`, `kernel/cc-runtime/`, `kernel/freestnd-c-hdrs-0bsd/` and `build/` folders and deletes `kernel/include/limine.h`.

### macOS

If you are on macOS, you will need to install Linux. If you are not building on Linux and are instead using macOS, do not expect any support for problems that are encountered.

### Windows

While really not recommended, you could use WSL. I still recommend you use Linux natively to build Arikoto.

## Contributions

While Arikoto is my personal OS project, bugfixes would help a lot. Major changes to functionality or new features to be added should be discussed first.

- [Discord Server](https://discord.gg/UczSZb7s7B)
- [GitHub Issues](https://github.com/IAmTheNerdNextDoor/arikoto/issues)
- [GitHub Discussions](https://github.com/IAmTheNerdNextDoor/arikoto/discussions)
- [Zed Collab](https://zed.dev/channel/Arikoto-19596)
