# Swift Inja Templates

This directory contains modular Inja templates for Swift code generation.

## Files

- **swift_example.inja** - Main template entry point (uses {% include %})
- **swift_single_file.inja** - Single-file version (no includes, for simpler usage)
- **_header.inja** - File header and structure documentation
- **_imports.inja** - Import statements
- **_metadata.inja** - MetaData struct generation
- **_viewmodel_with_databinding.inja** - ViewModel with data binding
- **_viewmodel_no_databinding.inja** - ViewModel without data binding
- **_properties_init.inja** - Properties initialization
- **_enums.inja** - Enum generation
- **_nested_properties.inja** - Nested property generation
- **_main_properties.inja** - Main property generation
- **_initializer_with_properties.inja** - Initializer with properties
- **_type_safe_switching.inja** - Type-safe switching extensions
- **_state_machines_metadata.inja** - State machine metadata
- **_state_machines_enum.inja** - State machine enum

## Using Modular Templates with Includes

Inja supports `{% include %}` directives out of the box. The include paths are resolved **relative to the current working directory** where you run the generator.

### Example

If you run the generator from `/Users/you/project/`:

```bash
cd /Users/you/project
rive_code_generator -i samples/file.riv -t templates/Swift/swift_example.inja -e inja
```

Then the includes in `swift_example.inja` must use paths relative to `/Users/you/project/`:

```inja
{% include "templates/Swift/_header.inja" %}
{% include "templates/Swift/_imports.inja" %}
```

### Choosing Between Modular and Single-File

- **swift_example.inja** - Use when you want modular, maintainable templates (recommended for large projects)
- **swift_single_file.inja** - Use when you want a simpler setup without worrying about working directory

Both templates generate identical output.
