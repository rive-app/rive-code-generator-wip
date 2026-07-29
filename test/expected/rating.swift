//
// RatingGenerated.swift
// Generated from rating.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from rating.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: Complete
// │  • State Machine: 
// │  • View Model: 
// │
// ├─ Artboards
// │  ├─ [DEFAULT] Complete
// │  │  ├─ Animation: Thumbnail
// │  │  ├─ Animation: 5_stars
// │  │  ├─ Animation: 4_stars
// │  │  ├─ Animation: 3_stars
// │  │  ├─ Animation: 2_stars
// │  │  ├─ Animation: 1_star
// │  │  ├─ Animation: Idle_empty
// │  │  ├─ State Machine: State Machine 1
// │
// └─ View Models
//


import RiveRuntime
import SwiftUI

// MARK: - Rating ViewModel

/// View model for rating.riv (no data binding)
@MainActor
public final class RiveRatingViewModel: RiveViewModel {

    // MARK: - MetaData

    /// Metadata for rating.riv
    public struct MetaData {

        /// The Rive file name
        public static let fileName = "rating"

    }
    // MARK: - Initializer

    public init(
        autoPlay: Bool = false,
        loadCdn: Bool = false,
        fit: RiveFit = .contain,
        alignment: RiveAlignment = .center,
        bundle: Bundle? = nil
    ) {
        super.init(
            configuration: .init(
                fileName: "rating",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            )
        )
    }
}
