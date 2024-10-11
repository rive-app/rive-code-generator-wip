# Rive Code Generator

A command-line tool to simplify the process of integrating Rive graphcis at runtime.

This tool allows you to parse Rive (`.riv`) files and extract component names, artboards, state machine inputs, etc in a human readable format. This tool can help you:
- Create type-safe wrappers for your `.riv` files, such as static helper classes
- Provides type-safe access to artboards, animations, and state machines
- Generate a `JSON` representation of your `.riv` files
- Diff `.riv` files for version control purposes
- Generate complete code components

This tool makes use of [Mustache](https://mustache.github.io/) templating.

## Build Instructions

### Prerequisites

- CMake (version 3.10 or higher)
- C++ compiler with C++17 support
- Rive C++ Runtime library

Run the build script:
```
cd build && ./build.sh
```

To build for release:
```
cd build && ./build.sh release
```

This will compile the code generator and create the executable in the `build/out/lib/debug` or `build/out/lib/release` directory.

## Usage

After building, run the code generator with the following syntax:

Debug:

```sh
./build/out/lib/debug/rive_code_generator -i <input_rive_file> -o <output_directory> -l <language>
```

Release:

```sh
./build/out/lib/release/rive_code_generator -i <input_rive_file> -o <output_directory> -l <language>
```

Replace `<input_rive_file>`, `<output_directory>`, and `<language>` with your specific values.

For example:

```sh
./build/rive_code_generator -i ./examples/rive_files/animation.riv -o ./examples/generated_code.dart -l dart
```

## Templates

Optionally, you can provide a custom template file to use for the generated code. Specify the template file with the `-t` option which the path to the file.

```sh
./build/rive_code_generator -i ./rive_files/ -o ./output/rive.json -t templates/json_template.mustache
```

For examples of custom templates, please check the [`templates`](./templates) directory in this repository. You'll find sample templates for various output formats and languages.

## License

MIT
