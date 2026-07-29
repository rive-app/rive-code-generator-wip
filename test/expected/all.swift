//
// TextRunsGenerated.swift
// Generated from textRuns.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from textRuns.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: Artboard-Nr1
// │  • State Machine: State Machine 1
// │  • View Model: 
// │
// ├─ Artboards
// │  ├─ [DEFAULT] Artboard-Nr1
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  ├─ Artboard-Nr2
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │
// └─ View Models
//


import RiveRuntime
import SwiftUI

// MARK: - TextRuns ViewModel

/// View model for textRuns.riv (no data binding)
@MainActor
public final class RiveTextRunsViewModel: RiveViewModel {

    // MARK: - MetaData

    /// Metadata for textRuns.riv
    public struct MetaData {

        /// The Rive file name
        public static let fileName = "text_runs"

        // MARK: - Artboards

        public enum Artboard: String, CaseIterable, Sendable {
            case artboardNr1 = "Artboard-Nr1"
            case artboardNr2 = "Artboard-Nr2"

            public var name: String {
                switch self {
                case .artboardNr1: return "Artboard-Nr1"
                case .artboardNr2: return "Artboard-Nr2"
                }
            }

            public static let explicitDefault: Artboard = .artboardNr1
        }


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
                fileName: "text_runs",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            )
        )
    }
}

// MARK: - Type-Safe Switching

public extension RiveTextRunsViewModel {

    func switchArtboard(to artboard: MetaData.Artboard) throws {
        try switchArtboard(to: artboard.name)
    }
}
//
// NestedTextGenerated.swift
// Generated from nestedText.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from nestedText.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: Artboard1
// │  • State Machine: State Machine 1
// │  • View Model: 
// │
// ├─ Artboards
// │  ├─ [DEFAULT] Artboard1
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  ├─ ArtboardDoNotFind
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  ├─ Artboard3
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  ├─ Artboard2
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │
// └─ View Models
//


import RiveRuntime
import SwiftUI

// MARK: - NestedText ViewModel

/// View model for nestedText.riv (no data binding)
@MainActor
public final class RiveNestedTextViewModel: RiveViewModel {

    // MARK: - MetaData

    /// Metadata for nestedText.riv
    public struct MetaData {

        /// The Rive file name
        public static let fileName = "nested_text"

        // MARK: - Artboards

        public enum Artboard: String, CaseIterable, Sendable {
            case artboard1 = "Artboard1"
            case artboarddonotfind = "ArtboardDoNotFind"
            case artboard3 = "Artboard3"
            case artboard2 = "Artboard2"

            public var name: String {
                switch self {
                case .artboard1: return "Artboard1"
                case .artboarddonotfind: return "ArtboardDoNotFind"
                case .artboard3: return "Artboard3"
                case .artboard2: return "Artboard2"
                }
            }

            public static let explicitDefault: Artboard = .artboard1
        }


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
                fileName: "nested_text",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            )
        )
    }
}

// MARK: - Type-Safe Switching

public extension RiveNestedTextViewModel {

    func switchArtboard(to artboard: MetaData.Artboard) throws {
        try switchArtboard(to: artboard.name)
    }
}
//
// RewardsGenerated.swift
// Generated from rewards.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from rewards.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: Main
// │  • State Machine: State Machine 1
// │  • View Model: Rewards
// │
// ├─ Artboards
// │  ├─ [DEFAULT] Main
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Rewards
// │  ├─ Lives 2
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Energy_Bar
// │  ├─ Item Value
// │  │  ├─ Animation: jump
// │  │  ├─ Animation: Idle
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Item_Icon_Value
// │  ├─ Chest
// │  │  ├─ Animation: Coin
// │  │  ├─ Animation: Gem
// │  │  ├─ Animation: Items_Move
// │  │  ├─ Animation: Items_Idle
// │  │  ├─ Animation: Open
// │  │  ├─ Animation: Idle
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Rewards
// │  ├─ Button
// │  │  ├─ Animation: Button_Base
// │  │  ├─ Animation: ReleaseCoins
// │  │  ├─ Animation: ReleaseGems
// │  │  ├─ Animation: Pressed
// │  │  ├─ Animation: Idle
// │  │  ├─ Animation: HitboxGems
// │  │  ├─ Animation: HitboxCoins
// │  │  ├─ Animation: State_2
// │  │  ├─ Animation: State_1
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Rewards
// │  ├─ Heart
// │  │  ├─ Animation: Down
// │  │  ├─ Animation: Top
// │  │  ├─ Animation: right
// │  │  ├─ Animation: Left
// │  │  ├─ Animation: Vertical
// │  │  ├─ Animation: Horizontal
// │  │  ├─ State Machine: State Machine 1
// │  ├─ Item
// │  │  ├─ Animation: Scale 0
// │  │  ├─ Animation: Scale 100
// │  │  ├─ Animation: Coin
// │  │  ├─ Animation: Gem
// │  │  ├─ Animation: Down
// │  │  ├─ Animation: Top
// │  │  ├─ Animation: right
// │  │  ├─ Animation: Left
// │  │  ├─ Animation: Vertical
// │  │  ├─ Animation: Horizontal
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Item
// │
// └─ View Models
//    ├─ [MAIN] Item_Icon_Value
//    │  ├─ Icon_React: Trigger
//    │  ├─ Property_Of_Item: Item (nested)
//    │  ├─ Item_Value: Float = 0.000000
//    ├─ Energy_Bar
//    │  ├─ Bar_Color: UIColor= 0xFF000000
//    │  ├─ Lives: Float = 0.000000
//    │  ├─ Energy_Bar: Float = 0.000000
//    ├─ Rewards
//    │  ├─ Price_Value: Float = 0.000000
//    │  ├─ Color: UIColor= 0xFF000000
//    │  ├─ Height: Float = 0.000000
//    │  ├─ With: Float = 0.000000
//    │  ├─ Item_Selection: Item (nested)
//    │  ├─ Item_Value_Icon: ItemIconValue (nested)
//    │  ├─ Button: Button (nested)
//    │  ├─ Coin: ItemIconValue (nested)
//    │  ├─ Gem: ItemIconValue (nested)
//    │  ├─ Energy_Bar: EnergyBar (nested)
//    ├─ Button
//    │  ├─ State_1: String = ""
//    │  ├─ Item_Text: String = ""
//    │  ├─ Item: Item (nested)
//    │  ├─ Pressed: Trigger
//    ├─ Item
//    │  ├─ Item_Selection: ItemSelectionEnum = .coin
//


import RiveRuntime
import SwiftUI

// MARK: - Rewards ViewModel

/// View model for rewards.riv with data binding support
///
/// **Usage:**
/// ```swift SwiftUI
/// @StateObject var viewModel = RiveRewardsViewModel()
/// viewModel.properties.someProperty = value
/// viewModel.play()
/// ```
/// ```swift UIKit
/// let viewModel = RiveRewardsViewModel()
/// viewModel.properties.someProperty = value
/// viewModel.play()
/// ```
@MainActor
public final class RiveRewardsViewModel: RiveViewModel {

    // MARK: - MetaData

    /// Metadata for rewards.riv
    public struct MetaData {

        /// The Rive file name
        public static let fileName = "rewards"

        // MARK: - Artboards

        public enum Artboard: String, CaseIterable, Sendable {
            case main = "Main"
            case lives2 = "Lives 2"
            case itemValue = "Item Value"
            case chest = "Chest"
            case button = "Button"
            case heart = "Heart"
            case item = "Item"

            public var name: String {
                switch self {
                case .main: return "Main"
                case .lives2: return "Lives 2"
                case .itemValue: return "Item Value"
                case .chest: return "Chest"
                case .button: return "Button"
                case .heart: return "Heart"
                case .item: return "Item"
                }
            }

            public static let explicitDefault: Artboard = .main
        }


    }
    // MARK: - Properties

    public struct Properties: RiveDataBindingProperties {

        public init() {}

        public init(
            Item_Value: Float? = nil
) {
            self.init()
            if let Item_Value {
                self.Item_Value = Item_Value
            }
}

        // MARK: - Enums

        public enum ItemSelectionEnum: String, CaseIterable, Sendable {
            case coin = "Coin"
            case gem = "Gem"

            public var displayName: String {
                return rawValue
            }
        }

        // MARK: - EnergyBar Nested Properties

        public struct EnergyBarProperties: RiveDataBindingProperties {

            public init() {}
@ColorProperty("Bar_Color") public var Bar_Color: UIColor?
@NumberProperty("Lives") public var Lives: Float?
@NumberProperty("Energy_Bar") public var Energy_Bar: Float?
        }

        // MARK: - Rewards Nested Properties

        public struct RewardsProperties: RiveDataBindingProperties {

            public init() {}
@NumberProperty("Price_Value") public var Price_Value: Float?
@ColorProperty("Color") public var Color: UIColor?
@NumberProperty("Height") public var Height: Float?
@NumberProperty("With") public var With: Float?
@ViewModelProperty<ItemProperties>("Item_Selection") public var Item_Selection: ViewModelProperty<ItemProperties>
@ViewModelProperty<ItemIconValueProperties>("Item_Value_Icon") public var Item_Value_Icon: ViewModelProperty<ItemIconValueProperties>
@ViewModelProperty<ButtonProperties>("Button") public var Button: ViewModelProperty<ButtonProperties>
@ViewModelProperty<ItemIconValueProperties>("Coin") public var Coin: ViewModelProperty<ItemIconValueProperties>
@ViewModelProperty<ItemIconValueProperties>("Gem") public var Gem: ViewModelProperty<ItemIconValueProperties>
@ViewModelProperty<EnergyBarProperties>("Energy_Bar") public var Energy_Bar: ViewModelProperty<EnergyBarProperties>
        }

        // MARK: - Button Nested Properties

        public struct ButtonProperties: RiveDataBindingProperties {

            public init() {}
@StringProperty("State_1") public var State_1: String?
@StringProperty("Item_Text") public var Item_Text: String?
@ViewModelProperty<ItemProperties>("Item") public var Item: ViewModelProperty<ItemProperties>
@TriggerProperty("Pressed") public var Pressed: TriggerProperty
        }

        // MARK: - Item Nested Properties

        public struct ItemProperties: RiveDataBindingProperties {

            public init() {}
@EnumProperty<ItemSelectionEnum>("Item_Selection") public var Item_Selection: ItemSelectionEnum?
        }

        // MARK: - Main Properties
@TriggerProperty("Icon_React") public var Icon_React: TriggerProperty
@ViewModelProperty<ItemProperties>("Property_Of_Item") public var Property_Of_Item: ViewModelProperty<ItemProperties>
@NumberProperty("Item_Value") public var Item_Value: Float?
    }

    // MARK: - Initializer

    public init(
        autoPlay: Bool = false,
        loadCdn: Bool = false,
        fit: RiveFit = .contain,
        alignment: RiveAlignment = .center,
        bundle: Bundle? = nil,
        properties: Properties = Properties()
    ) {
        super.init(
            configuration: .init(
                fileName: "rewards",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            ),
            properties: properties
        )
    }
}

// MARK: - Type-Safe Switching

public extension RiveRewardsViewModel {

    func switchArtboard(to artboard: MetaData.Artboard) throws {
        try switchArtboard(to: artboard.name)
    }
}
//
// DbGeneratorGenerated.swift
// Generated from dbGenerator.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from dbGenerator.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: Artboard
// │  • State Machine: State Machine 1
// │  • View Model: Person
// │
// ├─ Artboards
// │  ├─ [DEFAULT] Artboard
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │  │  └─ View Model: Person
// │
// └─ View Models
//    ├─ [MAIN] Pet
//    │  ├─ pet_type: PetEnum = .snake
//    │  ├─ name: String = ""
//    │  ├─ age: Float = 0.000000
//    ├─ Person
//    │  ├─ my_pet: Pet (nested)
//    │  ├─ age: Float = 0.000000
//    │  ├─ jump: Trigger
//    │  ├─ color: UIColor= 0xFF000000
//    │  ├─ isTall: Bool = false
//    │  ├─ name: String = ""
//


import RiveRuntime
import SwiftUI

// MARK: - DbGenerator ViewModel

/// View model for dbGenerator.riv with data binding support
///
/// **Usage:**
/// ```swift SwiftUI
/// @StateObject var viewModel = RiveDbGeneratorViewModel()
/// viewModel.properties.someProperty = value
/// viewModel.play()
/// ```
/// ```swift UIKit
/// let viewModel = RiveDbGeneratorViewModel()
/// viewModel.properties.someProperty = value
/// viewModel.play()
/// ```
@MainActor
public final class RiveDbGeneratorViewModel: RiveViewModel {

    // MARK: - Properties

    public struct Properties: RiveDataBindingProperties {

        public init() {}

        public init(
            pet_type: PetEnum? = nil,
            name: String? = nil,
            age: Float? = nil
) {
            self.init()
            if let pet_type {
                self.pet_type = pet_type
            }
            if let name {
                self.name = name
            }
            if let age {
                self.age = age
            }
}

        // MARK: - Enums

        public enum PetEnum: String, CaseIterable, Sendable {
            case snake = "Snake"
            case cat = "Cat"
            case dog = "Dog"

            public var displayName: String {
                return rawValue
            }
        }

        // MARK: - Person Nested Properties

        public struct PersonProperties: RiveDataBindingProperties {

            public init() {}
@ViewModelProperty<PetProperties>("my_pet") public var my_pet: ViewModelProperty<PetProperties>
@NumberProperty("age") public var age: Float?
@TriggerProperty("jump") public var jump: TriggerProperty
@ColorProperty("color") public var color: UIColor?
@BooleanProperty("isTall") public var isTall: Bool?
@StringProperty("name") public var name: String?
        }

        // MARK: - Main Properties
@EnumProperty<PetEnum>("pet_type") public var pet_type: PetEnum?
@StringProperty("name") public var name: String?
@NumberProperty("age") public var age: Float?
    }

    // MARK: - Initializer

    public init(
        autoPlay: Bool = false,
        loadCdn: Bool = false,
        fit: RiveFit = .contain,
        alignment: RiveAlignment = .center,
        bundle: Bundle? = nil,
        properties: Properties = Properties()
    ) {
        super.init(
            configuration: .init(
                fileName: "db_generator",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            ),
            properties: properties
        )
    }
}
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
//
// FileAssetsGenerated.swift
// Generated from fileAssets.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from fileAssets.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: Artboard
// │  • State Machine: State Machine 1
// │  • View Model: 
// │
// ├─ Artboards
// │  ├─ [DEFAULT] Artboard
// │  │  ├─ Animation: Timeline 1
// │  │  ├─ State Machine: State Machine 1
// │
// └─ View Models
//


import RiveRuntime
import SwiftUI

// MARK: - FileAssets ViewModel

/// View model for fileAssets.riv (no data binding)
@MainActor
public final class RiveFileAssetsViewModel: RiveViewModel {
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
                fileName: "file_assets",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            )
        )
    }
}
//
// LittleMachineGenerated.swift
// Generated from littleMachine.riv
//
// ⚠️ DO NOT EDIT - This file is auto-generated from littleMachine.riv
// Regenerate with: make rive-codegen
//
// MARK: - Rive File Structure
//
// ┌─ Default Configuration
// │  • Artboard: New Artboard
// │  • State Machine: 
// │  • View Model: 
// │
// ├─ Artboards
// │  ├─ [DEFAULT] New Artboard
// │  │  ├─ Animation: Idle light
// │  │  ├─ Animation: Idle
// │  │  ├─ Animation: Work
// │  │  ├─ Animation: Insert_data
// │  │  ├─ State Machine: State Machine 1
// │
// └─ View Models
//


import RiveRuntime
import SwiftUI

// MARK: - LittleMachine ViewModel

/// View model for littleMachine.riv (no data binding)
@MainActor
public final class RiveLittleMachineViewModel: RiveViewModel {

    // MARK: - MetaData

    /// Metadata for littleMachine.riv
    public struct MetaData {

        /// The Rive file name
        public static let fileName = "little_machine"

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
                fileName: "little_machine",
                autoPlay: autoPlay,
                loadCdn: loadCdn,
                fit: fit,
                alignment: alignment,
                bundle: bundle ?? .module
            )
        )
    }
}
