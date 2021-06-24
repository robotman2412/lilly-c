# lilly-c
The beginnings of a C compiler for simple 8- and 16-bit CPUs.

## What is it?
A simple C compiler for simple CPUs (think of custom CPUs, the MOS 6502 or the 8086).
In addition, an architecture may support assembly files and inline assembly.

## What is it not?
An advanced C/C++ compiler with endless optimisations.
This compiler does not do C++.

## Developing for lilly-c
1. Pull git git repository: `git clone https://github.com/robotman2412/lilly-c`
2. Configure (see [Configuration](#Configuration))
3. Build for the first time: `./build.sh` <sup>(1)</sup>
4. The output executable is called `comp` <sup>(2)</sup>

Note 1: The initial build is required because several files are generated.

Note 2: Executable name subject to change.

## Configuration
There are two options at this point:

### Configure for an existing architecture:
1. To list help and architectures: `./configure.sh --help`
2. Pick an architecture and configure: `./configure.sh --arch=mos6502`

Note: MOS 6502 is not implemented at the moment.

### Create a new architecture:
1. Think of an appropriate architecture ID, for example: `8086`
1. Create a directory with the desired architecture ID: `src/arch/8086`
2. Create a `decription.txt` there, for example: `Intel 8086 desktop CPU` <sup>(1)</sup>
3. Write `8086-config.h`, `8086-gen.c` and `8086-types.h` accordingly. <sup>(2)</sup>

Note 1: Do not add a newline to `description.txt`, it must ideally be a single line.

Note 2: Refer to `src/arch/gr8cpu-r3` for examples of implementing these files.
