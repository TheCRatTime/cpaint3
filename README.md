# cpaint2
**cpaint2** - highlight your C code.

## Description
There is a clean version of [cpaint2](https://github.com/TheCRatTime/cpaint2/).
**hyperfine** results:
```text
$ hyperfine --warmup 3 './cpaint3 src/cpaint3.c' 'cpaint2 src/cpaint3.c' 'cpaint src/cpaint3.c'

Benchmark 1: ./cpaint3 src/cpaint3.c (201 runs)
                   mean ±     σ    min …   max
  Wall Time [ms]   15.2 ±   3.1   12.3 …  27.6
  Memory [MiB]      1.5 ±   0.0    1.5 …   1.6

Benchmark 2: cpaint2 src/cpaint3.c (89 runs)
                   mean ±     σ    min …   max
  Wall Time [ms]   33.9 ±   4.1   29.6 …  54.1  +122.6%  (2.2x slower)
  Memory [MiB]      1.7 ±   0.0    1.6 …   1.7    +7.1%

Benchmark 3: cpaint src/cpaint3.c (128 runs)
                   mean ±     σ    min …   max
  Wall Time [ms]   22.6 ±   2.9   19.8 …  32.4   +48.6%  (1.5x slower)
  Memory [MiB]      1.7 ±   0.0    1.7 …   1.7    +9.4%
```

### Features
- **C89**/**ANSI C** code standard.
- **Smart painting**: using lexer for full interpretation.

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
