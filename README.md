# Arikoto (ありこと)
[![CodeQL Runner](https://github.com/AFellowSpeedrunner/arikoto/actions/workflows/codeql.yml/badge.svg)](https://github.com/AFellowSpeedrunner/arikoto/actions/workflows/codeql.yml) ![loc](https://tokei.rs/b1/github/IAmTheNerdNextDoor/arikoto)

Arikoto is my personal Operating System development project (which I'm hoping will be usable in the future) that is inspired by and is aiming to have a similar nature to [ToaruOS](https://github.com/klange/toaruos) (which is my favourite hobby OS out there and I may be slightly obsessed with it).

## Features/Current Stuff:
UEFI and BIOS x86_64 support using Limine protocol.

Written in C (with partial help from artificial intelligence because obviously I'm not the best programmer in the world but I'm not fully writing the code with AI because that would be stupid and I wouldn't learn anything).

Some memory support and listing upon boot.
Simple VFS/ramdisk functionality.
Working scheduler but also not really.
Full CMake build system with seperated includes and src.
Keyboard support and minimal shell.

## Building and Testing

### Linux

Run `mkdir build && cd build` in the Arikoto root folder. Type `cmake ..`. The project will be configured automatically.

From here, you can type `make`, `make run`, `make run-kvm`, `make run-uefi` and (my favourite) `make run-uefi-kvm`.

Running `make clean` wipes built binaries and arikoto.iso if an image was built. Running `make distclean` wipes the `limine/`, `ovmf/`, `kernel/cc-runtime/`, `kernel/freestnd-c-hdrs-0bsd/` and `build/` folders and deletes `kernel/include/limine.h`.

### macOS

However, if you are on macOS, you will need to install Linux.

If you are not building on Linux and are instead using macOS, do not expect any support for problems that are encountered.

### Windows

While really not recommended, you could use WSL.

I still recommend you use Linux natively to build Arikoto.

## Ways to Contribute

While Arikoto is my personal OS project, helping to fix bugs and issues in Arikoto helps a lot. Any major changes to functionality or any new features to be added need to be discussed using the methods below.

Main conversation related to Arikoto will take place on the [Zed code editor channel](https://zed.dev/channel/Arikoto-19596), on the [Discord server](https://discord.gg/UczSZb7s7B) and on the GitHub Discussions.

## Why CMake for an OS project?

It's expansive and flexible for me. It's something I wanted to experiment and use more, so why not use it with a personal experimental project?
