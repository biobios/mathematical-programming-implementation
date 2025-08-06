# Mathematical Programming Implementation

## Projects

- mpilib
- eaxlib
- eax
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
make bin/libmpilib.a
# debug compilation (with -g option)
make bin/debug/libmpilib.a
# profile compilation (with -pg option)
make bin/prof/libmpilib.a
```

### eaxlib
Directory: `./src/libeaxlib`

This project contains the utilities for the EAX algorithm.

#### How to use make
To compile the `eaxlib`:
```bash
# normal compilation
make bin/libeaxlib.a
# debug compilation (with -g option)
make bin/debug/libeaxlib.a
# profile compilation (with -pg option)
make bin/prof/libeaxlib.a
```

### eax
Directory: `./src/eax`

This project implements the EAX algorithm for ATSP.

#### How to use make
To compile the `eax`:
```bash
# normal compilation
make bin/eax
# debug compilation (with -g option)
make bin/debug/eax
# profile compilation (with -pg option)
make bin/prof/eax
```

To run the `eax`:
```bash
# normal run
make run/eax
# profile run
make run/prof/eax
```

### eax_stsp
Directory: `./src/eax_stsp`

This project implements the EAX algorithm for STSP.

#### How to use make
To compile the `eax_stsp`:
```bash
# normal compilation
make bin/eax_stsp
# debug compilation (with -g option)
make bin/debug/eax_stsp
# profile compilation (with -pg option)
make bin/prof/eax_stsp
```

To run the `eax_stsp`:
```bash
# normal run
ARGS="ARGUMENTS" make run/eax_stsp
# profile run
ARGS="ARGUMENTS" make run/prof/eax_stsp
```
`--help` can be used to see the available arguments.

### fast_eax
Directory: `./src/fast_eax`

This project implements a faster version of the EAX algorithm.

#### How to use make
To compile the `fast_eax`:
```bash
# normal compilation
make bin/fast_eax
# debug compilation (with -g option)
make bin/debug/fast_eax
# profile compilation (with -pg option)
make bin/prof/fast_eax
```

To run the `fast_eax`:
```bash
# normal run
ARGS="ARGUMENTS" make run/fast_eax
# profile run
ARGS="ARGUMENTS" make run/prof/fast_eax
```
`--help` can be used to see the available arguments.

### simplex
Directory: `./src/simplex`

This project implements the Simplex algorithm.

#### How to use make
To compile the `simplex`:
```bash
# normal compilation
make bin/simplex
# debug compilation (with -g option)
make bin/debug/simplex
# profile compilation (with -pg option)
make bin/prof/simplex
```

To run the `simplex`:
```bash
# normal run
make run/simplex
# profile run
make run/prof/simplex
```
