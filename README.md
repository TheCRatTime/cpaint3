# cpaint3
**cpaint3** - highlight your C code (version 3).

## Description
This is a clean version of [cpaint2](https://github.com/TheCRatTime/cpaint2/).
**hyperfine 2.0.0** results:
```text
$ hyperfine --warmup 3 'cpaint3 src/cpaint3.c' 'cpaint2 src/cpaint3.c' 'cpaint src/cpaint3.c'

Benchmark 1: cpaint3 src/cpaint3.c (180 runs)
                   mean ±     σ    min …   max
  Wall Time [ms]   16.6 ±   3.4   12.8 …  29.7
  Memory [MiB]      1.6 ±   0.0    1.5 …   1.6

Benchmark 2: cpaint2 src/cpaint3.c (79 runs)
                   mean ±     σ    min …   max
  Wall Time [ms]   38.6 ±   3.1   34.2 …  46.3  +132.1%  (2.3x slower)
  Memory [MiB]      1.7 ±   0.0    1.6 …   1.7    +6.6%

Benchmark 3: cpaint src/cpaint3.c (116 runs)
                   mean ±     σ    min …   max
  Wall Time [ms]   26.6 ±   4.2   22.6 …  45.7   +60.1%  (1.6x slower)
  Memory [MiB]      1.7 ±   0.0    1.7 …   1.7    +9.4%
```

### Features
- **C89**/**ANSI C** code standard.
- **Smart painting**: using lexer for full interpretation.
- **Big keyword base**: supports **63** keywords.

### Build
Needed:
- C Compiler
- CMake 3.10 and greater
- GNU Make

```bash
cmake -B build
cd build/
make
```

### Authors
TheCRatTime

### License
Project is under Apache 2.0 license.
