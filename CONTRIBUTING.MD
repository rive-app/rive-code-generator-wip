# Contributing to Rive Code Generator

## Cloning

```sh
git clone --recurse-submodules https://github.com/rive-app/rive-code-generator-wip
```

or

```sh
git clone https://github.com/rive-app/rive-code-generator-wip
cd rive-code-generator-wip
git submodule update --init --recursive
```

## Build Instructions

### Prerequisites

- CMake (version 3.10 or higher)
- C++ compiler with C++17 support
- Rive C++ Runtime library

### Building

To build the debug version:
```sh
cd build && ./build.sh
```

For the release version:
```sh
cd build && ./build.sh release
```

The executable will be created in `build/out/lib/debug` or `build/out/lib/release`.

## Testing

To run the tests:
```sh
cd test && ./test.sh
```

Add additional tests to the `test.sh` script.

To regenerate the test data:
```sh
cd test && ./test.sh --update-expected
```