#pragma once

namespace default_templates
{

    /// Default Dart template
    const char *DEFAULT_DART_TEMPLATE = R"(
// {{generated_file_name}}.dart

// ignore_for_file: lines_longer_than_80_chars, unused_field

// ignore: avoid_classes_with_only_static_members
/// Main entry point for accessing Rive metadata.
/// This class provides static getters for each Rive file's metadata.
abstract class RiveMeta {
  {{#riv_files}}
  /// Getter for the metadata of the {{riv_camel_case}} Rive file.
  static const _{{riv_pascal_case}}File {{riv_camel_case}} = _{{riv_pascal_case}}File();

  {{/riv_files}}
}

{{#riv_files}}
// ----------------------
// {{riv_camel_case}} Rive file
// ----------------------

/// Metadata for the "{{riv_camel_case}}" Rive file.
class _{{riv_pascal_case}}File {
  const _{{riv_pascal_case}}File();

  _{{riv_pascal_case}}Artboards get artboards => const _{{riv_pascal_case}}Artboards();
}

/// Container for all artboards in the "{{riv_camel_case}}" Rive file.
class _{{riv_pascal_case}}Artboards {
  const _{{riv_pascal_case}}Artboards();

  {{#artboards}}
  _{{riv_pascal_case}}Artboard{{artboard_pascal_case}} get {{artboard_camel_case}} => const _{{riv_pascal_case}}Artboard{{artboard_pascal_case}}();
  {{/artboards}}
}

{{#artboards}}
/// Metadata for the "{{artboard_camel_case}}" artboard.
class _{{riv_pascal_case}}Artboard{{artboard_pascal_case}} {
  const _{{riv_pascal_case}}Artboard{{artboard_pascal_case}}();

  String get name => '{{artboard_name}}';

  _{{riv_pascal_case}}{{artboard_pascal_case}}Animations get animations => const _{{riv_pascal_case}}{{artboard_pascal_case}}Animations();
  _{{riv_pascal_case}}{{artboard_pascal_case}}StateMachines get stateMachines => const _{{riv_pascal_case}}{{artboard_pascal_case}}StateMachines();
}

/// Container for animation names in the "{{artboard_camel_case}}" artboard.
class _{{riv_pascal_case}}{{artboard_pascal_case}}Animations {
  const _{{riv_pascal_case}}{{artboard_pascal_case}}Animations();

  {{#animations}}
  String get {{animation_camel_case}} => '{{animation_name}}';
  {{/animations}}
}

/// Container for state machines in the "{{artboard_camel_case}}" artboard.
class _{{riv_pascal_case}}{{artboard_pascal_case}}StateMachines {
  const _{{riv_pascal_case}}{{artboard_pascal_case}}StateMachines();

  {{#state_machines}}
  _{{riv_pascal_case}}{{artboard_pascal_case}}{{state_machine_camel_case}} get {{state_machine_camel_case}} => const _{{riv_pascal_case}}{{artboard_pascal_case}}{{state_machine_camel_case}}();
  {{/state_machines}}
}

{{#state_machines}}
/// Metadata for the "{{state_machine_camel_case}}" state machine.
class _{{riv_pascal_case}}{{artboard_pascal_case}}{{state_machine_camel_case}} {
  const _{{riv_pascal_case}}{{artboard_pascal_case}}{{state_machine_camel_case}}();

  String get name => '{{state_machine_name}}';
}
{{/state_machines}}

{{/artboards}}

{{/riv_files}}
)";

} // namespace default_templates