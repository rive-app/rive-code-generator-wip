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
  /// Getter for the metadata of the {{riv_variable}} Rive file.
  static const _{{riv_class}}File {{riv_variable}} = _{{riv_class}}File();

  {{/riv_files}}
}

{{#riv_files}}
// ----------------------
// {{riv_variable}} Rive file
// ----------------------

/// Metadata for the "{{riv_variable}}" Rive file.
class _{{riv_class}}File {
  const _{{riv_class}}File();

  _{{riv_class}}Artboards get artboards => const _{{riv_class}}Artboards();
}

/// Container for all artboards in the "{{riv_variable}}" Rive file.
class _{{riv_class}}Artboards {
  const _{{riv_class}}Artboards();

  {{#artboards}}
  _{{riv_class}}Artboard{{artboard_class_name}} get {{artboard_variable}} => const _{{riv_class}}Artboard{{artboard_class_name}}();
  {{/artboards}}
}

{{#artboards}}
/// Metadata for the "{{artboard_variable}}" artboard.
class _{{riv_class}}Artboard{{artboard_class_name}} {
  const _{{riv_class}}Artboard{{artboard_class_name}}();

  String get name => '{{artboard_name}}';

  _{{riv_class}}{{artboard_class_name}}Animations get animations => const _{{riv_class}}{{artboard_class_name}}Animations();
  _{{riv_class}}{{artboard_class_name}}StateMachines get stateMachines => const _{{riv_class}}{{artboard_class_name}}StateMachines();
}

/// Container for animation names in the "{{artboard_variable}}" artboard.
class _{{riv_class}}{{artboard_class_name}}Animations {
  const _{{riv_class}}{{artboard_class_name}}Animations();

  {{#animations}}
  String get {{animation_variable}} => '{{animation_name}}';
  {{/animations}}
}

/// Container for state machines in the "{{artboard_variable}}" artboard.
class _{{riv_class}}{{artboard_class_name}}StateMachines {
  const _{{riv_class}}{{artboard_class_name}}StateMachines();

  {{#state_machines}}
  _{{riv_class}}{{artboard_class_name}}{{state_machine_variable}} get {{state_machine_variable}} => const _{{riv_class}}{{artboard_class_name}}{{state_machine_variable}}();
  {{/state_machines}}
}

{{#state_machines}}
/// Metadata for the "{{state_machine_variable}}" state machine.
class _{{riv_class}}{{artboard_class_name}}{{state_machine_variable}} {
  const _{{riv_class}}{{artboard_class_name}}{{state_machine_variable}}();

  String get name => '{{state_machine_name}}';
}
{{/state_machines}}

{{/artboards}}

{{/riv_files}}
)";

} // namespace default_templates