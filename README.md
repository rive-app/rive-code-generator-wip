# Rive Code Generator

A command-line tool to simplify the integration of Rive graphics at runtime.

This tool parses Rive (`.riv`) files and extracts component names, artboards, state machine inputs, and other elements in a human-readable format. Key features include:

- Creating type-safe wrappers for `.riv` files (e.g., static helper classes)
- Providing type-safe access to artboards, animations, and state machines
- Generating JSON representations of `.riv` files
- Diffing `.riv` files for version control purposes
- Generating complete code components

The tool uses [Mustache](https://mustache.github.io/) templating for flexible output generation.

:warning: Note that this tool is still experimental and untested. Feedback and contributions are appreciated.

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

## Usage

Run the code generator using:

```sh
./build/out/lib/<debug|release>/rive_code_generator -i <input_rive_file> -o <output_directory> -l <language>
```

Example:
```sh
./build/out/lib/release/rive_code_generator -i ./examples/rive_files/animation.riv -o ./examples/generated_code.dart -l dart
```

## Custom Templates

You can use custom Mustache templates for code generation:

```sh
./build/out/lib/release/rive_code_generator -i ./rive_files/ -o ./output/rive.json -t templates/json_template.mustache
```

Sample templates are available in the [`templates`](./templates) directory.

### Template Syntax

The tool uses [Mustache](https://mustache.github.io/) templating. Please refer to the [Mustache documentation](https://mustache.github.io/) for syntax details.

For each Rive file `{{#riv_files}}`, the following variables are available:
- `{{riv_pascal_case}}`: The Rive file name in PascalCase
- `{{riv_camel_case}}`: The Rive file name in camelCase
- `{{riv_snake_case}}`: The Rive file name in snake_case
- `{{riv_kebab_case}}`: The Rive file name in kebab-case
- For each artboard `{{#artboards}}`:
    - `{{artboard_name}}`: The original artboard name
    - `{{artboard_pascal_case}}`: The artboard name in PascalCase
    - `{{artboard_camel_case}}`: The artboard name in camelCase
    - `{{artboard_snake_case}}`: The artboard name in snake_case
    - `{{artboard_kebab_case}}`: The artboard name in kebab-case
    - `{{animations}}`: List of animation names for the artboard
    - For each animation `{{#animations}}`:
        - `{{animation_name}}`: Name of the animation
        - `{{animation_camel_case}}`: Name of the animation in camelCase
        - `{{animation_pascal_case}}`: Name of the animation in PascalCase
        - `{{animation_snake_case}}`: Name of the animation in snake_case
        - `{{animation_kebab_case}}`: Name of the animation in kebab-case
    - `{{state_machines}}`: List of state machines for the artboard
    - For each state machine `{{#state_machines}}`:
        - `{{state_machine_name}}`: Name of the state machine
        - `{{state_machine_camel_case}}`: Name of the state machine in camelCase
        - `{{state_machine_pascal_case}}`: Name of the state machine in PascalCase
        - `{{state_machine_snake_case}}`: Name of the state machine in snake_case
        - `{{state_machine_kebab_case}}`: Name of the state machine in kebab-case
        - `{{inputs}}`: List of inputs for the state machine
        - For each input `{{#inputs}}`:
            - `{{input_name}}`: Name of the input
            - `{{input_type}}`: Type of the input
            - `{{input_default_value}}`: Default value of the input
    - `{{text_value_runs}}`: List of text value runs for the artboard
    - For each text value run `{{#text_value_runs}}`:
        - `{{text_value_run_name}}`: Name of the text value run
        - `{{text_value_run_camel_case}}`: Name of the text value run in camelCase
        - `{{text_value_run_pascal_case}}`: Name of the text value run in PascalCase
        - `{{text_value_run_snake_case}}`: Name of the text value run in snake_case
        - `{{text_value_run_kebab_case}}`: Name of the text value run in kebab-case
        - `{{text_value_run_default}}`: Default value of the text value run
        - `{{text_value_run_default_sanitized}}`: Default value of the text value run with special characters encoded

**:warning: Warning:** For duplicated names (e.g., multiple artboards or animations with the same name), the original unique names will be preserved. However, the case-converted versions (such as camelCase, PascalCase, etc.) will have a unique identifier attached to avoid conflicts.

For example:
- Original names: "MyArtboard", "MyArtboard"
- Unique camelCase: "myArtboard", "myArtboardU1"

This ensures that all generated code and references remain unique and valid.

## Supported Languages

At the moment, the tool supports Dart and JSON outputs. More defualt exports will be added. However, you can easily add your own by providing a custom template.

## Contributing

Contributions are welcome! Please see our [Contributing Guidelines](CONTRIBUTING.md) for more details.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
