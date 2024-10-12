// rive_generated.dart

// ignore_for_file: lines_longer_than_80_chars, unused_field

// ignore: avoid_classes_with_only_static_members
/// Main entry point for accessing Rive metadata.
/// This class provides static getters for each Rive file's metadata.
abstract class RiveMeta {
  /// Getter for the metadata of the rating Rive file.
  static const _RatingFile rating = _RatingFile();

}

// ----------------------
// rating Rive file
// ----------------------

/// Metadata for the "rating" Rive file.
class _RatingFile {
  const _RatingFile();

  _RatingArtboards get artboards => const _RatingArtboards();
}

/// Container for all artboards in the "rating" Rive file.
class _RatingArtboards {
  const _RatingArtboards();

  _RatingArtboardComplete get complete => const _RatingArtboardComplete();
}

/// Metadata for the "complete" artboard.
class _RatingArtboardComplete {
  const _RatingArtboardComplete();

  String get name => 'Complete';

  _RatingCompleteAnimations get animations => const _RatingCompleteAnimations();
  _RatingCompleteStateMachines get stateMachines => const _RatingCompleteStateMachines();
}

/// Container for animation names in the "complete" artboard.
class _RatingCompleteAnimations {
  const _RatingCompleteAnimations();

  String get thumbnail => 'Thumbnail';
  String get n5Stars => '5_stars';
  String get n4Stars => '4_stars';
  String get n3Stars => '3_stars';
  String get n2Stars => '2_stars';
  String get n1Star => '1_star';
  String get idleEmpty => 'Idle_empty';
}

/// Container for state machines in the "complete" artboard.
class _RatingCompleteStateMachines {
  const _RatingCompleteStateMachines();

  _RatingCompletestateMachine1 get stateMachine1 => const _RatingCompletestateMachine1();
}

/// Metadata for the "stateMachine1" state machine.
class _RatingCompletestateMachine1 {
  const _RatingCompletestateMachine1();

  String get name => 'State Machine 1';
}
