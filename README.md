# Mathematical Programming Implementation

## Projects

- mpilib
- eaxlib
- eax
- normal_eax
- eax_tabu
- eax_stsp
- fast_eax
- simplex

### mpilib
Directory: `./src/libmpilib`

This project contains the utilities for mathematical programming.

#### How to use make
To compile the `mpilib`:
```bash
# normal compilation
make build-libmpilib
# debug compilation (with -g option)
make debug-build-libmpilib
# profile compilation (with -pg option)
make prof-build-libmpilib
```

### eaxlib
Directory: `./src/libeaxlib`

This project contains the utilities for the EAX algorithm.

#### How to use make
To compile the `eaxlib`:
```bash
# normal compilation
make build-libeaxlib
# debug compilation (with -g option)
make debug-build-libeaxlib
# profile compilation (with -pg option)
make prof-build-libeaxlib
```

### eax
Directory: `./src/eax`

This project implements the EAX algorithm.

#### How to use make
To compile the `eax`:
```bash
# normal compilation
make build-eax
# debug compilation (with -g option)
make debug-build-eax
# profile compilation (with -pg option)
make prof-build-eax
```

To run the `eax`:
```bash
# normal run
ARGS="ARGUMENTS" make run-eax
# profile run
ARGS="ARGUMENTS" make prof-run-eax
```

`--help` can be used to see the available arguments.

### normal_eax
Directory: `./src/normal_eax`

This project implements the version of the paper [A Powerful Genetic Algorithm Using Edge Assembly Crossover for the Traveling Salesman Problem](https://doi.org/10.1287/ijoc.1120.0506).

#### How to use make
To compile the `normal_eax`:
```bash
# normal compilation
make build-normal_eax
# debug compilation (with -g option)
make debug-build-normal_eax
# profile compilation (with -pg option)
make prof-build-normal_eax
```

To run the `normal_eax`:
```bash
# normal run
ARGS="ARGUMENTS" make run-normal_eax
# profile run
ARGS="ARGUMENTS" make prof-run-normal_eax
```
`--help` can be used to see the available arguments.

### eax_tabu
Directory: `./src/eax_tabu`

This project implements the EAX algorithm with tabu edge.

#### How to use make
To compile the `eax_tabu`:
```bash
# normal compilation
make build-eax_tabu
# debug compilation (with -g option)
make debug-build-eax_tabu
# profile compilation (with -pg option)
make prof-build-eax_tabu
```

To run the `eax_tabu`:
```bash
# normal run
ARGS="ARGUMENTS" make run-eax_tabu
# profile run
ARGS="ARGUMENTS" make prof-run-eax_tabu
```
`--help` can be used to see the available arguments.

### eax_stsp
Directory: `./src/eax_stsp`

This project implements the EAX algorithm for STSP.
(Legacy code, not actively maintained.)

#### How to use make
To compile the `eax_stsp`:
```bash
# normal compilation
make build-eax_stsp
# debug compilation (with -g option)
make debug-build-eax_stsp
# profile compilation (with -pg option)
make prof-build-eax_stsp
```

To run the `eax_stsp`:
```bash
# normal run
ARGS="ARGUMENTS" make run-eax_stsp
# profile run
ARGS="ARGUMENTS" make prof-run-eax_stsp
```
`--help` can be used to see the available arguments.

### fast_eax
Directory: `./src/fast_eax`

This project implements a faster version of the EAX algorithm.
(Legacy code, not actively maintained.)

#### How to use make
To compile the `fast_eax`:
```bash
# normal compilation
make build-fast_eax
# debug compilation (with -g option)
make debug-build-fast_eax
# profile compilation (with -pg option)
make prof-build-fast_eax
```

To run the `fast_eax`:
```bash
# normal run
ARGS="ARGUMENTS" make run-fast_eax
# profile run
ARGS="ARGUMENTS" make prof-run-fast_eax
```
`--help` can be used to see the available arguments.

### simplex
Directory: `./src/simplex`

This project implements the Simplex algorithm.

#### How to use make
To compile the `simplex`:
```bash
# normal compilation
make build-simplex
# debug compilation (with -g option)
make debug-build-simplex
# profile compilation (with -pg option)
make prof-build-simplex
```

To run the `simplex`:
```bash
# normal run
make run-simplex
# profile run
make prof-run-simplex
```
