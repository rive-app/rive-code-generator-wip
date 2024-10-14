<h1 align="center">
  <a href="https://github.com/rive-app/rive-code-generator-wip">
    Rive Code Generator
  </a>
</h1>

<p align="center">
  <strong>Simplify Rive integration at runtime:</strong><br>
  Generate type-safe wrappers for Rive files.
</p>

<p align="center">
  <a href="https://github.com/rive-app/rive-code-generator-wip/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="Rive Code Generator is released under the MIT license." />
  </a>
  <a href="https://github.com/rive-app/rive-code-generator-wip/actions">
    <img src="https://github.com/rive-app/rive-code-generator-wip/actions/workflows/build_rive_code_generator.yaml/badge.svg" alt="Current GitHub Actions build status." />
  </a>
  <a href="https://github.com/rive-app/rive-code-generator-wip/releases">
    <img src="https://img.shields.io/github/v/release/rive-app/rive-code-generator-wip" alt="Latest GitHub release." />
  </a>
  <a href="https://github.com/rive-app/rive-code-generator-wip/blob/main/CONTRIBUTING.md">
    <img src="https://img.shields.io/badge/PRs-welcome-brightgreen.svg" alt="PRs welcome!" />
  </a>
</p>

<h3 align="center">
  <a href="#releases">Releases</a>
  <span> · </span>
  <a href="#usage">Usage</a>
  <span> · </span>
  <a href="#custom-templates">Custom Templates</a>
  <span> · </span>
  <a href="#contributing">Contribute</a>
  <span> · </span>
  <a href="#license">License</a>
</h3>

This tool parses Rive (`.riv`) files and extracts component names, artboards, state machine inputs, and other components in a human-readable format. Key features include:

- Creating type-safe wrappers for `.riv` files (e.g., static helper classes)
- Generating JSON representations of `.riv` files
- Diffing `.riv` files for version control purposes
- Generating complete code components

The tool uses [Mustache](https://mustache.github.io/) templating for flexible output generation.

:warning: Note that this tool is still experimental and untested. Feedback and contributions are appreciated.

## Releases

See the [release workflow](.github/workflows/release_workflow.yaml) for release details.

The latest release can be downloaded from the [Releases](https://github.com/rive-app/rive-code-generator-wip/releases) page.

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

- `{{assets}}`: List of assets in the Rive file
- For each asset `{{#assets}}`:
    - `{{asset_name}}`: Name of the asset
    - `{{asset_camel_case}}`: Name of the asset in camelCase
    - `{{asset_pascal_case}}`: Name of the asset in PascalCase
    - `{{asset_snake_case}}`: Name of the asset in snake_case
    - `{{asset_kebab_case}}`: Name of the asset in kebab-case
    - `{{asset_type}}`: Type of the asset
    - `{{asset_id}}`: ID of the asset
    - `{{asset_cdn_uuid}}`: CDN UUID of the asset
    - `{{asset_cdn_base_url}}`: CDN base URL of the asset

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
    - For each nested text value run `{{#nested_text_value_runs}}`:
        - `{{nested_text_value_run_name}}`: Name of the nested text value run
        - `{{nested_text_value_run_path}}`: Path of the nested text value run

**:warning: Warning:** For duplicated names (e.g., multiple artboards, animations, or assets with the same name), the original unique names will be preserved. However, the case-converted versions (such as camelCase, PascalCase, etc.) will have a unique identifier attached to avoid conflicts.

For example:
- Original names: "MyArtboard", "MyArtboard"
- Unique camelCase: "myArtboard", "myArtboardU1"

This ensures that all generated code and references remain unique and valid.

## Supported Languages

At the moment, the tool supports Dart and JSON outputs. More defualt exports will be added. However, you can easily add your own by providing a custom template.

## Contribute

Contributions are welcome! Please see our [Contributing Guidelines](CONTRIBUTING.md) for more details.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
