#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "CLIUTILS/CLI11.hpp"
#include "default_template.h"
#include "kainjow/mustache.hpp"
#include "inja.hpp"
#include "rive/animation/linear_animation_instance.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/assets/audio_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
#include "rive/file.hpp"
#include "rive/generated/animation/state_machine_bool_base.hpp"
#include "rive/generated/animation/state_machine_number_base.hpp"
#include "rive/generated/animation/state_machine_trigger_base.hpp"
#include "rive/viewmodel/data_enum.hpp"
#include "rive/viewmodel/data_enum_value.hpp"
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
#include "rive/viewmodel/viewmodel_property_enum.hpp"
#include "utils/no_op_factory.hpp"

const std::string generatedFileName = "rive_generated";

enum class CaseStyle
{
    Camel,
    Pascal,
    Snake,
    Kebab,
};

enum class Language
{
    Dart,
    JavaScript
};

enum class TemplateEngine
{
    Mustache,
    Inja
};

struct InputInfo
{
    std::string name;
    std::string type;
    std::string defaultValue;
};

struct TextValueRunInfo
{
    std::string name;
    std::string defaultValue;
};

struct NestedTextValueRunInfo
{
    std::string name;
    std::string path;
};

struct AssetInfo
{
    std::string name;
    std::string type;
    std::string fileExtension;
    std::string assetId;
    std::string cdnUuid;
    std::string cdnBaseUrl;
};

struct EnumValueInfo
{
    std::string key;
};

struct EnumInfo
{
    std::string name;
    std::vector<EnumValueInfo> values;
};

struct PropertyInfo
{
    std::string name;
    std::string type;
    std::string backingName;
    std::string defaultValue; // For enums, stores the default enum value key
};

struct ViewModelInfo
{
    std::string name;
    std::vector<PropertyInfo> properties;
};

struct ArtboardData
{
    std::string artboardName;
    std::string artboardPascalCase;
    std::string artboardCameCase;
    std::string artboardSnakeCase;
    std::string artboardKebabCase;
    std::vector<std::string> animations;
    std::vector<std::pair<std::string, std::vector<InputInfo>>> stateMachines;
    std::vector<TextValueRunInfo> textValueRuns;
    std::vector<NestedTextValueRunInfo> nestedTextValueRuns;
    
    // Relationship information
    bool isDefault;
    uint32_t viewModelId;
    std::string viewModelName;
    bool hasViewModel;
    std::string defaultStateMachineName;
    bool hasDefaultStateMachine;
};

struct RiveFileData
{
    std::string rivOriginalFileName;
    std::string rivPascalCase;
    std::string rivCameCase;
    std::string riveSnakeCase;
    std::string rivKebabCase;
    std::vector<ArtboardData> artboards;
    std::vector<AssetInfo> assets;
    std::vector<EnumInfo> enums;
    std::vector<ViewModelInfo> viewmodels;
    
    // Default relationship chain
    std::string defaultArtboardName;
    std::string defaultStateMachineName;
    std::string defaultViewModelName;
    bool hasDefaults;
};

// Helper function to convert a string to the specified case style
static std::string toCaseHelper(const std::string& str, CaseStyle style)
{
    std::stringstream result;
    bool capitalizeNext = (style == CaseStyle::Pascal);
    bool firstChar = true;

    // Check if the first character is a digit
    if (std::isdigit(str[0]))
    {
        result << 'n';         // Prepend 'n' for number
        capitalizeNext = true; // Capitalize the first digit
        firstChar = false;
    }

    // Process the string
    for (size_t i = 0; i < str.length(); i++)
    {
        char c = str[i];

        if (std::isalnum(c))
        {
            if (capitalizeNext)
            {
                result << (char)std::toupper(c);
                capitalizeNext = false;
            }
            else
            {
                result << (style == CaseStyle::Pascal ? c
                                                      : (char)std::tolower(c));
            }
            firstChar = false;
        }
        else if (c == ' ' || c == '_' || c == '-')
        {
            if (!firstChar)
            {
                switch (style)
                {
                    case CaseStyle::Camel:
                    case CaseStyle::Pascal:
                        capitalizeNext = true;
                        break;
                    case CaseStyle::Snake:
                        result << '_';
                        break;
                    case CaseStyle::Kebab:
                        result << '-';
                        break;
                }
            }
        }
        // All other characters are ignored
    }

    // Ensure the result is not empty and starts with a letter
    std::string finalResult = result.str();
    if (finalResult.empty() || !std::isalpha(finalResult[0]))
    {
        finalResult = "X" + finalResult;
    }

    return finalResult;
}

static std::string toCamelCase(const std::string& str)
{
    std::string result = toCaseHelper(str, CaseStyle::Camel);
    // TODO: These handlers are generic to dart, we need to make something more
    // generic to handle all languages
    // Handle Dart reserved keywords
    if (result == "with" || result == "class" || result == "enum" ||
        result == "var" || result == "const" || result == "final" ||
        result == "static" || result == "void" || result == "int" ||
        result == "double" || result == "bool" || result == "String" ||
        result == "List" || result == "Map" || result == "dynamic" ||
        result == "null" || result == "true" || result == "false")
    {
        result = result + "Value";
    }
    return result;
}

static std::string toPascalCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Pascal);
}

static std::string toSnakeCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Snake);
}

static std::string toKebabCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Kebab);
}

static std::string sanitizeString(const std::string& input)
{
    std::string output;
    for (char c : input)
    {
        switch (c)
        {
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            case '\"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            default:
                if (std::isprint(c))
                {
                    output += c;
                }
                else
                {
                    char hex[7];
                    std::snprintf(hex,
                                  sizeof(hex),
                                  "\\u%04x",
                                  static_cast<unsigned char>(c));
                    output += hex;
                }
        }
    }
    return output;
}

static rive::rcp<rive::File> openFile(const char name[])
{
    FILE* f = fopen(name, "rb");
    if (!f)
    {
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    auto length = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::vector<uint8_t> bytes(length);

    if (fread(bytes.data(), 1, length, f) != length)
    {
        printf("Failed to read file into bytes array\n");
        return nullptr;
    }

    static rive::NoOpFactory gFactory;
    return rive::File::import(bytes, &gFactory);
}

static bool shouldIncludeElement(const std::string& name, bool ignorePrivate)
{
    if (!ignorePrivate)
    {
        return true;
    }

    // Check for underscore prefix
    if (!name.empty() && name[0] == '_')
    {
        return false;
    }

    // Check for "internal" or "private" prefix (case-insensitive)
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    if (lowerName.rfind("internal", 0) == 0)
    {
        return false;
    }

    if (lowerName.rfind("private", 0) == 0)
    {
        return false;
    }

    return true;
}

static std::vector<std::string> getAnimationsFromArtboard(
    rive::ArtboardInstance* artboard, bool ignorePrivate = false)
{
    std::vector<std::string> animations;
    auto animationCount = artboard->animationCount();
    for (int i = 0; i < animationCount; i++)
    {
        auto animation = artboard->animationAt(i);
        std::string animationName = animation->name();

        // Skip animations that start with internal/private/_
        if (!shouldIncludeElement(animationName, ignorePrivate))
        {
            continue;
        }

        animations.push_back(animationName);
    }
    return animations;
}

static std::vector<std::pair<std::string, std::vector<InputInfo>>>
getStateMachinesFromArtboard(rive::ArtboardInstance* artboard, bool ignorePrivate = false)
{
    std::vector<std::pair<std::string, std::vector<InputInfo>>> stateMachines;
    auto stateMachineCount = artboard->stateMachineCount();
    for (int i = 0; i < stateMachineCount; i++)
    {
        auto stateMachine = artboard->stateMachineAt(i);
        std::string stateMachineName = stateMachine->name();

        // Skip state machines that start with internal/private/_
        if (!shouldIncludeElement(stateMachineName, ignorePrivate))
        {
            continue;
        }

        std::vector<InputInfo> inputs;
        auto inputCount = stateMachine->inputCount();
        for (int j = 0; j < inputCount; j++)
        {
            auto input = stateMachine->input(j);

            std::string inputType;
            std::string defaultValue;

            // Determine the input type and default value
            switch (input->inputCoreType())
            {
                case rive::StateMachineNumberBase::typeKey:
                {
                    auto smiNumberInput = static_cast<rive::SMINumber*>(input);
                    inputType = "number";
                    defaultValue = std::to_string(smiNumberInput->value());
                    break;
                }
                case rive::StateMachineTriggerBase::typeKey:
                {
                    inputType = "trigger";
                    defaultValue = "false";
                    break;
                }
                case rive::StateMachineBoolBase::typeKey:
                {
                    auto smiBoolInput = static_cast<rive::SMIBool*>(input);
                    inputType = "boolean";
                    defaultValue = smiBoolInput->value() ? "true" : "false";
                    break;
                }
                default:
                {
                    inputType = "unknown";
                    defaultValue = "";
                    break;
                }
            }

            inputs.push_back({input->name(), inputType, defaultValue});
        }

        stateMachines.emplace_back(stateMachineName, inputs);
    }
    return stateMachines;
}

static std::vector<std::string> findRiveFiles(const std::string& path)
{
    std::vector<std::string> riveFile;

    if (std::filesystem::is_directory(path))
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.path().extension() == ".riv")
            {
                riveFile.push_back(entry.path().string());
            }
        }
    }
    else if (std::filesystem::path(path).extension() == ".riv")
    {
        riveFile.push_back(path);
    }

    return riveFile;
}

static std::string makeUnique(const std::string& base,
                       std::unordered_set<std::string>& usedNames)
{
    std::string uniqueName = base;
    int counter = 1;
    while (usedNames.find(uniqueName) != usedNames.end())
    {
        uniqueName = base + "U" + std::to_string(counter);
        counter++;
    }
    usedNames.insert(uniqueName);
    return uniqueName;
}

template <typename T = rive::Component>
void findAll(std::vector<T*>& results, rive::ArtboardInstance* artboard)
{
    for (auto object : artboard->objects())
    {
        if (object != nullptr && object->is<T>())
        {
            results.push_back(static_cast<T*>(object));
        }
    }
}

static std::vector<TextValueRunInfo> getTextValueRunsFromArtboard(
    rive::ArtboardInstance* artboard)
{
    std::vector<rive::TextValueRun*> textValueRuns;
    std::vector<TextValueRunInfo> textValueRunsInfo;

    findAll<rive::TextValueRun>(textValueRuns, artboard);

    for (auto textValueRun : textValueRuns)
    {
        if (!textValueRun->name().empty())
        {
            textValueRunsInfo.push_back(
                {textValueRun->name(), textValueRun->text()});
        }
    }
    return textValueRunsInfo;
}

static std::vector<NestedTextValueRunInfo>
getNestedTextValueRunPathsFromArtboard(
    rive::ArtboardInstance* artboard,
    const std::string& currentPath = "")
{
    std::vector<NestedTextValueRunInfo> nestedTextValueRunsInfo;
    auto count = artboard->nestedArtboards().size();

    if (!currentPath.empty())
    {
        auto textRuns = getTextValueRunsFromArtboard(artboard);
        for (const auto& textRun : textRuns)
        {
            nestedTextValueRunsInfo.push_back(
                {textRun.name, currentPath});
        }
    }

    // Recursively process nested artboards
    for (int i = 0; i < count; i++)
    {
        auto nested = artboard->nestedArtboards()[i];
        auto nestedName = nested->name();
        if (!nestedName.empty())
        {
            // Only process nested artboards that have an exported name
            std::string newPath = currentPath.empty()
                                       ? nested->name()
                                       : currentPath + "/" + nested->name();

            auto nestedResults = getNestedTextValueRunPathsFromArtboard(
                nested->artboardInstance(),
                newPath);
            nestedTextValueRunsInfo.insert(
                nestedTextValueRunsInfo.end(),
                nestedResults.begin(),
                nestedResults.end());
        }
    }

    return nestedTextValueRunsInfo;
}

static std::vector<AssetInfo> getAssetsFromFile(rive::File* file)
{
    std::vector<AssetInfo> assetsInfo;
    std::unordered_set<std::string> usedAssetNames;

    auto assets = file->assets();
    for (auto asset : assets)
    {
        std::string assetType;
        switch (asset->coreType())
        {
            case rive::ImageAsset::typeKey:
                assetType = "image";
                break;
            case rive::FontAsset::typeKey:
                assetType = "font";
                break;
            case rive::AudioAsset::typeKey:
                assetType = "audio";
                break;
            default:
                assetType = "unknown";
                break;
        }

        auto assetName = asset->name();
        auto uniqueAssetName = makeUnique(assetName, usedAssetNames);

        assetsInfo.push_back(AssetInfo{uniqueAssetName,
                                       assetType,
                                       asset->fileExtension(),
                                       std::to_string(asset->assetId()),
                                       asset->cdnUuidStr(),
                                       asset->cdnBaseUrl()});
    }
    return assetsInfo;
}

static std::string dataTypeToString(rive::DataType type)
{
    switch (type)
    {
        case rive::DataType::none:
            return "none";
        case rive::DataType::string:
            return "string";
        case rive::DataType::number:
            return "number";
        case rive::DataType::boolean:
            return "boolean";
        case rive::DataType::color:
            return "color";
        case rive::DataType::list:
            return "list";
        case rive::DataType::enumType:
            return "enum";
        case rive::DataType::trigger:
            return "trigger";
        case rive::DataType::viewModel:
            return "viewModel";
        case rive::DataType::integer:
            return "integer";
        case rive::DataType::symbolListIndex:
            return "symbolListIndex";
        case rive::DataType::assetImage:
            return "assetImage";
        default:
            return "unknown";
    }
}

static std::optional<RiveFileData> processRiveFile(const std::string& riveFilePath, bool ignorePrivate = false)
{
    // Check if the file is empty
    if (std::filesystem::is_empty(riveFilePath))
    {
        std::cerr << "Error: Rive file is empty: " << riveFilePath
                  << std::endl;
        return std::nullopt;
    }

    auto riveFile = openFile(riveFilePath.c_str());
    if (!riveFile)
    {
        std::cerr << "Error: Failed to parse Rive file: " << riveFilePath
                  << std::endl;
        return std::nullopt;
    }

    std::filesystem::path path(riveFilePath);
    std::string fileNameWithoutExtension = path.stem().string();
    std::vector<AssetInfo> assets = getAssetsFromFile(riveFile.get());
    RiveFileData fileData;
    fileData.rivOriginalFileName = fileNameWithoutExtension;  // Preserve original filename
    fileData.rivPascalCase = toPascalCase(fileNameWithoutExtension);
    fileData.rivCameCase = toCamelCase(fileNameWithoutExtension);
    fileData.riveSnakeCase = toSnakeCase(fileNameWithoutExtension);
    fileData.rivKebabCase = toKebabCase(fileNameWithoutExtension);
    fileData.assets = assets;

    // Process enums
    const auto& fileEnums = riveFile->enums();
    for (auto* dataEnum : fileEnums)
    {
        if (dataEnum)
        {
            EnumInfo enumInfo;
            enumInfo.name = dataEnum->enumName();
            const auto& values = dataEnum->values();
            for (const auto* value : values)
            {
                enumInfo.values.push_back({value->key()});
            }
            fileData.enums.push_back(enumInfo);
        }
    }

    // Track which enums are actually used by non-filtered ViewModels
    std::unordered_set<std::string> usedEnumNames;

    // Process view models
    for (size_t i = 0; i < riveFile->viewModelCount(); i++)
    {
        auto viewModel = riveFile->viewModelByIndex(i);
        if (viewModel)
        {
            // Skip view models that start with internal/private/_
            if (!shouldIncludeElement(viewModel->name(), ignorePrivate))
            {
                continue;
            }

            ViewModelInfo viewModelInfo;
            viewModelInfo.name = viewModel->name();
            auto propertiesData = viewModel->properties();
            for (const auto& property : propertiesData)
            {
                // Skip properties that start with internal/private/_
                if (!shouldIncludeElement(property.name, ignorePrivate))
                {
                    continue;
                }

                if (property.type == rive::DataType::viewModel)
                {
                    // TODO: this is a hack
                    auto nestedViewModel =
                        viewModel->createInstance()->propertyViewModel(
                            property.name);
                    auto vm = nestedViewModel->instance()->viewModel();

                    // Skip nested ViewModels that have private names
                    if (!shouldIncludeElement(vm->name(), ignorePrivate))
                    {
                        continue;
                    }

                    viewModelInfo.properties.push_back(
                        {property.name,
                         dataTypeToString(property.type),
                         vm->name(),
                         ""}); // ViewModels don't have default values
                }
                else if (property.type == rive::DataType::enumType)
                {
                    // TODO: this is a hack
                    auto vmi =
                        riveFile->createViewModelInstance(viewModel->name());
                    auto enum_instance =
                        static_cast<rive::ViewModelInstanceEnum*>(
                            vmi->propertyValue(property.name));
                    auto enumProperty = enum_instance->viewModelProperty()
                                            ->as<rive::ViewModelPropertyEnum>();
                    auto enumName = enumProperty->dataEnum()->enumName();

                    // Track that this enum is used by a non-filtered ViewModel
                    usedEnumNames.insert(enumName);

                    // Get the default value from the enum instance
                    uint32_t defaultIndex = enum_instance->propertyValue();
                    auto dataEnum = enumProperty->dataEnum();
                    std::string defaultValue = "";
                    if (defaultIndex < dataEnum->values().size()) {
                        defaultValue = dataEnum->values()[defaultIndex]->key();
                    }

                    viewModelInfo.properties.push_back(
                        {property.name,
                         dataTypeToString(property.type),
                         enumName,
                         defaultValue});
                }
                else
                {
                    // Get default values for other property types
                    auto vmi = riveFile->createViewModelInstance(viewModel->name());
                    std::string defaultValue = "";

                    if (property.type == rive::DataType::boolean) {
                        auto bool_instance = static_cast<rive::ViewModelInstanceBoolean*>(
                            vmi->propertyValue(property.name));
                        defaultValue = bool_instance->propertyValue() ? "true" : "false";
                    }
                    else if (property.type == rive::DataType::number) {
                        auto number_instance = static_cast<rive::ViewModelInstanceNumber*>(
                            vmi->propertyValue(property.name));
                        defaultValue = std::to_string(number_instance->propertyValue());
                    }
                    else if (property.type == rive::DataType::string) {
                        auto string_instance = static_cast<rive::ViewModelInstanceString*>(
                            vmi->propertyValue(property.name));
                        defaultValue = string_instance->propertyValue();
                    }
                    else if (property.type == rive::DataType::color) {
                        auto color_instance = static_cast<rive::ViewModelInstanceColor*>(
                            vmi->propertyValue(property.name));
                        int colorValue = color_instance->propertyValue();
                        // Format as hex string (0xAARRGGBB)
                        std::stringstream ss;
                        ss << "0x" << std::hex << std::uppercase << std::setfill('0')
                           << std::setw(8) << static_cast<unsigned int>(colorValue);
                        defaultValue = ss.str();
                    }
                    else if (property.type == rive::DataType::assetImage) {
                        // Image properties don't have extractable default values
                        defaultValue = "";
                    }
                    // Trigger and other types don't have default values

                    viewModelInfo.properties.push_back(
                        {property.name, dataTypeToString(property.type), "", defaultValue});
                }
            }
            fileData.viewmodels.push_back(viewModelInfo);
        }
    }

    // Filter enums to only include those used by non-filtered ViewModels
    // If ignorePrivate is enabled and we have ViewModels, only keep enums that are actually used
    if (ignorePrivate && !fileData.viewmodels.empty())
    {
        std::vector<EnumInfo> filteredEnums;
        for (const auto& enumInfo : fileData.enums)
        {
            // Keep enum if it's used by a non-filtered ViewModel
            if (usedEnumNames.count(enumInfo.name) > 0)
            {
                filteredEnums.push_back(enumInfo);
            }
        }
        fileData.enums = filteredEnums;
    }

    // Extract default relationship chain
    auto defaultArtboard = riveFile->artboard(); // First artboard is default
    fileData.hasDefaults = (defaultArtboard != nullptr);
    
    if (fileData.hasDefaults) {
        fileData.defaultArtboardName = defaultArtboard->name();
        
        // Get default state machine for the default artboard
        auto defaultStateMachineInstance = defaultArtboard->instance()->defaultStateMachine();
        if (defaultStateMachineInstance) {
            fileData.defaultStateMachineName = defaultStateMachineInstance->name();
        }
        
        // Get default viewmodel for the default artboard
        auto defaultViewModelRuntime = riveFile->defaultArtboardViewModel(defaultArtboard);
        if (defaultViewModelRuntime) {
            fileData.defaultViewModelName = defaultViewModelRuntime->name();
        }
    }

    std::unordered_set<std::string> usedArtboardNames;

    auto artboardCount = riveFile->artboardCount();
    for (int i = 0; i < artboardCount; i++)
    {
        auto artboard = riveFile->artboardAt(i);
        std::string artboardName = artboard->name();

        // Skip artboards that start with internal/private/_
        if (!shouldIncludeElement(artboardName, ignorePrivate))
        {
            continue;
        }

        std::string artboardPascalCase = toPascalCase(artboardName);
        std::string artboardCameCase = toCamelCase(artboardName);
        std::string artboardSnakeCase = toSnakeCase(artboardName);
        std::string artboardKebabCase = toKebabCase(artboardName);

        // Ensure unique artboard variable names
        artboardCameCase =
            makeUnique(artboardCameCase, usedArtboardNames);

        std::vector<std::string> animations =
            getAnimationsFromArtboard(artboard.get(), ignorePrivate);
        std::vector<std::pair<std::string, std::vector<InputInfo>>>
            stateMachines = getStateMachinesFromArtboard(artboard.get(), ignorePrivate);
        std::vector<TextValueRunInfo> textValueRuns =
            getTextValueRunsFromArtboard(artboard.get());
        std::vector<NestedTextValueRunInfo> nestedTextValueRuns =
            getNestedTextValueRunPathsFromArtboard(artboard.get());

        // Extract relationship information for this artboard
        bool isDefault = (i == 0); // First artboard is default
        uint32_t artboardViewModelId = artboard->viewModelId();
        bool hasViewModel = (artboardViewModelId < fileData.viewmodels.size());
        std::string viewModelName = "";
        if (hasViewModel) {
            viewModelName = fileData.viewmodels[artboardViewModelId].name;
        }
        
        // Get default state machine for this artboard
        std::string defaultStateMachineName = "";
        bool hasDefaultStateMachine = false;
        auto artboardInstance = artboard->instance();
        if (artboardInstance) {
            auto defaultStateMachine = artboardInstance->defaultStateMachine();
            if (defaultStateMachine) {
                defaultStateMachineName = defaultStateMachine->name();
                hasDefaultStateMachine = true;
            }
        }

        fileData.artboards.push_back({artboardName,
                                       artboardPascalCase,
                                       artboardCameCase,
                                       artboardSnakeCase,
                                       artboardKebabCase,
                                       animations,
                                       stateMachines,
                                       textValueRuns,
                                       nestedTextValueRuns,
                                       isDefault,
                                       artboardViewModelId,
                                       viewModelName,
                                       hasViewModel,
                                       defaultStateMachineName,
                                       hasDefaultStateMachine});
    }

    return fileData;
}

static std::optional<std::string> readTemplateFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Warning: Unable to open template file: " << path
                  << std::endl;
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

// Helper function to build JSON data for inja from RiveFileData
static nlohmann::json buildInjaData(const std::vector<RiveFileData>& riveFileDataList)
{
    nlohmann::json data;
    data["generated_file_name"] = generatedFileName;

    nlohmann::json riveFileList = nlohmann::json::array();

    for (size_t fileIndex = 0; fileIndex < riveFileDataList.size(); fileIndex++)
    {
        const auto& fileData = riveFileDataList[fileIndex];
        nlohmann::json riveFileData;

        riveFileData["riv_original_file_name"] = fileData.rivOriginalFileName;
        riveFileData["riv_pascal_case"] = fileData.rivPascalCase;
        riveFileData["riv_camel_case"] = fileData.rivCameCase;
        riveFileData["riv_snake_case"] = fileData.riveSnakeCase;
        riveFileData["riv_kebab_case"] = fileData.rivKebabCase;
        riveFileData["last"] = (fileIndex == riveFileDataList.size() - 1);

        // Add default relationship chain
        riveFileData["has_defaults"] = fileData.hasDefaults;
        riveFileData["default_artboard_name"] = fileData.defaultArtboardName;
        riveFileData["default_artboard_camel_case"] = toCamelCase(fileData.defaultArtboardName);
        riveFileData["default_state_machine_name"] = fileData.defaultStateMachineName;
        riveFileData["default_view_model_name"] = fileData.defaultViewModelName;

        // Add enums
        nlohmann::json enums = nlohmann::json::array();
        for (size_t enumIndex = 0; enumIndex < fileData.enums.size(); enumIndex++)
        {
            const auto& enumInfo = fileData.enums[enumIndex];
            nlohmann::json enumData;
            enumData["enum_name"] = enumInfo.name;
            enumData["enum_camel_case"] = toCamelCase(enumInfo.name);
            enumData["enum_pascal_case"] = toPascalCase(enumInfo.name);
            enumData["enum_snake_case"] = toSnakeCase(enumInfo.name);
            enumData["enum_kebab_case"] = toKebabCase(enumInfo.name);
            enumData["last"] = (enumIndex == fileData.enums.size() - 1);

            nlohmann::json enumValues = nlohmann::json::array();
            for (size_t valueIndex = 0; valueIndex < enumInfo.values.size(); valueIndex++)
            {
                const auto& value = enumInfo.values[valueIndex];
                nlohmann::json valueData;
                const auto enumValueCamel = toCamelCase(value.key);
                valueData["enum_value_key"] = value.key;
                valueData["enum_value_camel_case"] = enumValueCamel;
                valueData["enum_value_pascal_case"] = toPascalCase(value.key);
                valueData["enum_value_snake_case"] = toSnakeCase(value.key);
                valueData["enum_value_kebab_case"] = toKebabCase(value.key);
                if (value.key != enumValueCamel)
                {
                    valueData["enum_value_needs_explicit_value"] = true;
                }
                valueData["last"] = (valueIndex == enumInfo.values.size() - 1);
                enumValues.push_back(valueData);
            }
            enumData["enum_values"] = enumValues;
            enums.push_back(enumData);
        }
        riveFileData["enums"] = enums;

        // Add view models
        nlohmann::json viewmodels = nlohmann::json::array();
        for (size_t vmIndex = 0; vmIndex < fileData.viewmodels.size(); vmIndex++)
        {
            const auto& viewModel = fileData.viewmodels[vmIndex];
            nlohmann::json viewmodelData;
            viewmodelData["view_model_name"] = viewModel.name;
            viewmodelData["view_model_camel_case"] = toCamelCase(viewModel.name);
            viewmodelData["view_model_pascal_case"] = toPascalCase(viewModel.name);
            viewmodelData["view_model_snake_case"] = toSnakeCase(viewModel.name);
            viewmodelData["view_model_kebab_case"] = toKebabCase(viewModel.name);
            viewmodelData["last"] = (vmIndex == fileData.viewmodels.size() - 1);
            viewmodelData["is_first"] = (vmIndex == 0);

            nlohmann::json properties = nlohmann::json::array();
            for (size_t propIndex = 0; propIndex < viewModel.properties.size(); propIndex++)
            {
                const auto& property = viewModel.properties[propIndex];
                nlohmann::json propertyData;
                propertyData["property_name"] = property.name;
                propertyData["property_camel_case"] = toCamelCase(property.name);
                propertyData["property_pascal_case"] = toPascalCase(property.name);
                propertyData["property_snake_case"] = toSnakeCase(property.name);
                propertyData["property_kebab_case"] = toKebabCase(property.name);
                propertyData["property_type"] = property.type;

                // Add property type information
                nlohmann::json propertyTypeData;
                propertyTypeData["is_view_model"] = (property.type == "viewModel");
                propertyTypeData["is_enum"] = (property.type == "enum");
                propertyTypeData["is_string"] = (property.type == "string");
                propertyTypeData["is_number"] = (property.type == "number");
                propertyTypeData["is_integer"] = (property.type == "integer");
                propertyTypeData["is_boolean"] = (property.type == "boolean");
                propertyTypeData["is_color"] = (property.type == "color");
                propertyTypeData["is_list"] = (property.type == "list");
                propertyTypeData["is_image"] = (property.type == "image" || property.type == "assetImage");
                propertyTypeData["is_trigger"] = (property.type == "trigger");
                propertyTypeData["backing_name"] = property.backingName;
                propertyTypeData["backing_camel_case"] = toCamelCase(property.backingName);
                propertyTypeData["backing_pascal_case"] = toPascalCase(property.backingName);
                propertyTypeData["backing_snake_case"] = toSnakeCase(property.backingName);
                propertyTypeData["backing_kebab_case"] = toKebabCase(property.backingName);

                if (!property.defaultValue.empty()) {
                    propertyTypeData["default_value"] = property.defaultValue;

                    if (property.type == "enum") {
                        propertyTypeData["enum_default_value"] = property.defaultValue;
                        propertyTypeData["enum_default_value_camel"] = toCamelCase(property.defaultValue);
                    }
                }

                propertyData["property_type"] = propertyTypeData;
                propertyData["last"] = (propIndex == viewModel.properties.size() - 1);
                properties.push_back(propertyData);
            }
            viewmodelData["properties"] = properties;
            viewmodels.push_back(viewmodelData);
        }
        riveFileData["view_models"] = viewmodels;

        // Add assets
        nlohmann::json assets = nlohmann::json::array();
        for (size_t assetIndex = 0; assetIndex < fileData.assets.size(); assetIndex++)
        {
            const auto& asset = fileData.assets[assetIndex];
            nlohmann::json assetData;
            assetData["asset_name"] = asset.name;
            assetData["asset_camel_case"] = toCamelCase(asset.name);
            assetData["asset_pascal_case"] = toPascalCase(asset.name);
            assetData["asset_snake_case"] = toSnakeCase(asset.name);
            assetData["asset_kebab_case"] = toKebabCase(asset.name);
            assetData["asset_type"] = asset.type;
            assetData["asset_id"] = asset.assetId;
            assetData["asset_cdn_uuid"] = asset.cdnUuid;
            assetData["asset_cdn_base_url"] = asset.cdnBaseUrl;
            assetData["last"] = (assetIndex == fileData.assets.size() - 1);
            assets.push_back(assetData);
        }
        riveFileData["assets"] = assets;

        // Add artboards
        nlohmann::json artboardList = nlohmann::json::array();
        for (size_t artboardIndex = 0; artboardIndex < fileData.artboards.size(); artboardIndex++)
        {
            const auto& artboard = fileData.artboards[artboardIndex];
            nlohmann::json artboardData;
            artboardData["artboard_name"] = artboard.artboardName;
            artboardData["artboard_pascal_case"] = artboard.artboardPascalCase;
            artboardData["artboard_camel_case"] = artboard.artboardCameCase;
            artboardData["artboard_snake_case"] = artboard.artboardSnakeCase;
            artboardData["artboard_kebab_case"] = artboard.artboardKebabCase;
            artboardData["last"] = (artboardIndex == fileData.artboards.size() - 1);

            // Add relationship information
            artboardData["is_default"] = artboard.isDefault;
            artboardData["view_model_id"] = static_cast<int>(artboard.viewModelId);
            artboardData["view_model_name"] = artboard.viewModelName;
            artboardData["has_view_model"] = artboard.hasViewModel;
            artboardData["default_state_machine_name"] = artboard.defaultStateMachineName;
            artboardData["has_default_state_machine"] = artboard.hasDefaultStateMachine;

            // Add animations
            std::unordered_set<std::string> usedAnimationNames;
            nlohmann::json animations = nlohmann::json::array();
            for (size_t animIndex = 0; animIndex < artboard.animations.size(); animIndex++)
            {
                const auto& animation = artboard.animations[animIndex];
                nlohmann::json animData;
                auto uniqueName = makeUnique(animation, usedAnimationNames);
                animData["animation_name"] = animation;
                animData["animation_camel_case"] = toCamelCase(uniqueName);
                animData["animation_pascal_case"] = toPascalCase(uniqueName);
                animData["animation_snake_case"] = toSnakeCase(uniqueName);
                animData["animation_kebab_case"] = toKebabCase(uniqueName);
                animData["last"] = (animIndex == artboard.animations.size() - 1);
                animations.push_back(animData);
            }
            artboardData["animations"] = animations;

            // Add state machines
            std::unordered_set<std::string> usedStateMachineNames;
            nlohmann::json stateMachines = nlohmann::json::array();
            for (size_t smIndex = 0; smIndex < artboard.stateMachines.size(); smIndex++)
            {
                const auto& stateMachine = artboard.stateMachines[smIndex];
                nlohmann::json stateMachineData;
                auto uniqueName = makeUnique(stateMachine.first, usedStateMachineNames);
                stateMachineData["state_machine_name"] = stateMachine.first;
                stateMachineData["state_machine_camel_case"] = toCamelCase(uniqueName);
                stateMachineData["state_machine_pascal_case"] = toPascalCase(uniqueName);
                stateMachineData["state_machine_snake_case"] = toSnakeCase(uniqueName);
                stateMachineData["state_machine_kebab_case"] = toKebabCase(uniqueName);
                stateMachineData["last"] = (smIndex == artboard.stateMachines.size() - 1);

                // Add inputs
                std::unordered_set<std::string> usedInputNames;
                nlohmann::json inputs = nlohmann::json::array();
                for (size_t inputIndex = 0; inputIndex < stateMachine.second.size(); inputIndex++)
                {
                    const auto& input = stateMachine.second[inputIndex];
                    nlohmann::json inputData;
                    auto uniqueName = makeUnique(input.name, usedInputNames);
                    inputData["input_name"] = input.name;
                    inputData["input_camel_case"] = toCamelCase(uniqueName);
                    inputData["input_pascal_case"] = toPascalCase(uniqueName);
                    inputData["input_snake_case"] = toSnakeCase(uniqueName);
                    inputData["input_kebab_case"] = toKebabCase(uniqueName);
                    inputData["input_type"] = input.type;
                    inputData["input_default_value"] = input.defaultValue;
                    inputData["last"] = (inputIndex == stateMachine.second.size() - 1);
                    inputs.push_back(inputData);
                }
                stateMachineData["inputs"] = inputs;
                stateMachines.push_back(stateMachineData);
            }
            artboardData["state_machines"] = stateMachines;

            // Add text value runs
            std::unordered_set<std::string> usedTextValueRunNames;
            nlohmann::json textValueRuns = nlohmann::json::array();
            for (size_t tvrIndex = 0; tvrIndex < artboard.textValueRuns.size(); tvrIndex++)
            {
                const auto& tvr = artboard.textValueRuns[tvrIndex];
                nlohmann::json tvrData;
                auto uniqueName = makeUnique(tvr.name, usedTextValueRunNames);
                tvrData["text_value_run_name"] = tvr.name;
                tvrData["text_value_run_camel_case"] = toCamelCase(uniqueName);
                tvrData["text_value_run_pascal_case"] = toPascalCase(uniqueName);
                tvrData["text_value_run_snake_case"] = toSnakeCase(uniqueName);
                tvrData["text_value_run_kebab_case"] = toKebabCase(uniqueName);
                tvrData["text_value_run_default"] = tvr.defaultValue;
                tvrData["text_value_run_default_sanitized"] = sanitizeString(tvr.defaultValue);
                tvrData["last"] = (tvrIndex == artboard.textValueRuns.size() - 1);
                textValueRuns.push_back(tvrData);
            }
            artboardData["text_value_runs"] = textValueRuns;

            // Add nested text value runs
            nlohmann::json nestedTextValueRuns = nlohmann::json::array();
            for (size_t ntvrIndex = 0; ntvrIndex < artboard.nestedTextValueRuns.size(); ntvrIndex++)
            {
                const auto& ntvr = artboard.nestedTextValueRuns[ntvrIndex];
                nlohmann::json ntvrData;
                ntvrData["nested_text_value_run_name"] = ntvr.name;
                ntvrData["nested_text_value_run_path"] = ntvr.path;
                ntvrData["last"] = (ntvrIndex == artboard.nestedTextValueRuns.size() - 1);
                nestedTextValueRuns.push_back(ntvrData);
            }
            artboardData["nested_text_value_runs"] = nestedTextValueRuns;

            artboardList.push_back(artboardData);
        }
        riveFileData["artboards"] = artboardList;

        // Add count flags
        riveFileData["artboard_count"] = fileData.artboards.size();
        riveFileData["has_multiple_artboards"] = fileData.artboards.size() > 1;

        size_t totalAnimations = 0;
        size_t totalStateMachines = 0;
        for (const auto& artboard : fileData.artboards) {
            totalAnimations += artboard.animations.size();
            totalStateMachines += artboard.stateMachines.size();
        }
        riveFileData["total_animation_count"] = totalAnimations;
        riveFileData["has_multiple_animations"] = totalAnimations > 1;
        riveFileData["total_state_machine_count"] = totalStateMachines;
        riveFileData["has_state_machines"] = totalStateMachines > 0;
        riveFileData["has_multiple_state_machines"] = totalStateMachines > 1;

        bool hasMetadata = fileData.artboards.size() > 1 || totalAnimations > 1 || totalStateMachines > 1;
        riveFileData["has_metadata"] = hasMetadata;

        bool hasViewModel = !fileData.viewmodels.empty();
        riveFileData["has_view_model"] = hasViewModel;

        bool hasTypeSafeSwitching = fileData.artboards.size() > 1 ||
                                     (!hasViewModel && totalStateMachines == 0 && totalAnimations > 1) ||
                                     totalStateMachines > 1;
        riveFileData["has_type_safe_switching"] = hasTypeSafeSwitching;

        riveFileList.push_back(riveFileData);
    }

    data["riv_files"] = riveFileList;
    return data;
}

int main(int argc, char* argv[])
{
    CLI::App app{"Rive Code Generator"};

    std::string inputPath;
    std::string outputFilePath;
    std::string templatePath;
    Language language = Language::Dart; // Default to Dart
    TemplateEngine templateEngine = TemplateEngine::Mustache; // Default to Mustache for backwards compatibility
    bool ignorePrivate = false;

    app.add_option("-i, --input",
                   inputPath,
                   "Path to Rive file or directory containing Rive files")
        ->required()
        ->check(CLI::ExistingFile | CLI::ExistingDirectory);

    app.add_option("-o, --output", outputFilePath, "Output file path")
        ->required();

    app.add_option("-t,--template", templatePath, "Custom template file path");

    app.add_option("-l, --language",
                   language,
                   "Programming language for code generation")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, Language>{{"dart", Language::Dart},
                                            {"js", Language::JavaScript}},
            CLI::ignore_case));

    app.add_option("-e, --engine",
                   templateEngine,
                   "Template engine to use (mustache or inja)")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, TemplateEngine>{{"mustache", TemplateEngine::Mustache},
                                                   {"inja", TemplateEngine::Inja}},
            CLI::ignore_case));

    app.add_flag("--ignore-private",
                 ignorePrivate,
                 "Skip artboards, animations, state machines, and properties starting with 'internal', 'private', or '_'");

    CLI11_PARSE(app, argc, argv)

    std::string templateStr;
    if (!templatePath.empty())
    {
        auto customTemplate = readTemplateFile(templatePath);
        if (customTemplate)
        {
            templateStr = *customTemplate;
            std::cout << "Using custom template from: " << templatePath
                      << std::endl;
        }
        else
        {
            // TODO: This is probably not needed. Or can have a safety to
            // fallback to the language specified
            std::cout << "Falling back to default template." << std::endl;
            templateStr = default_templates::DEFAULT_DART_TEMPLATE;
        }
    }
    else
    {
        if (language == Language::Dart)
        {
            templateStr = default_templates::DEFAULT_DART_TEMPLATE;
        }
        else if (language == Language::JavaScript)
        {
            std::cout << "JavaScript code generation is not yet supported."
                      << std::endl;
            return 1;
        }
    }

    std::vector<std::string> riveFiles = findRiveFiles(inputPath);

    if (riveFiles.empty())
    {
        std::cerr << "No .riv files found in the specified path." << std::endl;
        return 1;
    }

    std::vector<RiveFileData> riveFileDataList;
    for (const auto& riv_file : riveFiles)
    {
        auto result = processRiveFile(riv_file, ignorePrivate);
        if (result)
        {
            riveFileDataList.push_back(*result);
        }
        // If result is nullopt, the error has already been printed
    }

    // Mustache template rendering
    kainjow::mustache::data templateData;
    std::vector<kainjow::mustache::data> riveFileList;

    for (size_t fileIndex = 0; fileIndex < riveFileDataList.size();
         fileIndex++)
    {
        const auto& fileData = riveFileDataList[fileIndex];
        kainjow::mustache::data riveFileData;
        riveFileData["riv_original_file_name"] = fileData.rivOriginalFileName;
        riveFileData["riv_pascal_case"] = fileData.rivPascalCase;
        riveFileData["riv_camel_case"] = fileData.rivCameCase;
        riveFileData["riv_snake_case"] = fileData.riveSnakeCase;
        riveFileData["riv_kebab_case"] = fileData.rivKebabCase;
        riveFileData["last"] = (fileIndex == riveFileDataList.size() - 1);
        
        // Add default relationship chain
        riveFileData["has_defaults"] = fileData.hasDefaults;
        riveFileData["default_artboard_name"] = fileData.defaultArtboardName;
        riveFileData["default_artboard_camel_case"] = toCamelCase(fileData.defaultArtboardName);
        riveFileData["default_state_machine_name"] = fileData.defaultStateMachineName;
        riveFileData["default_view_model_name"] = fileData.defaultViewModelName;

        // Add enums to template data
        std::vector<kainjow::mustache::data> enums;
        for (size_t enumIndex = 0; enumIndex < fileData.enums.size();
             enumIndex++)
        {
            const auto& enumInfo = fileData.enums[enumIndex];
            kainjow::mustache::data enumData;
            enumData["enum_name"] = enumInfo.name;
            enumData["enum_camel_case"] = toCamelCase(enumInfo.name);
            enumData["enum_pascal_case"] = toPascalCase(enumInfo.name);
            enumData["enum_snake_case"] = toSnakeCase(enumInfo.name);
            enumData["enum_kebab_case"] = toKebabCase(enumInfo.name);
            enumData["last"] = (enumIndex == fileData.enums.size() - 1);

            std::vector<kainjow::mustache::data> enumValues;
            for (size_t valueIndex = 0; valueIndex < enumInfo.values.size();
                 valueIndex++)
            {
                const auto& value = enumInfo.values[valueIndex];
                kainjow::mustache::data valueData;
                const auto enumValueCamel = toCamelCase(value.key);
                valueData["enum_value_key"] = value.key;
                valueData["enum_value_camel_case"] = enumValueCamel;
                valueData["enum_value_pascal_case"] = toPascalCase(value.key);
                valueData["enum_value_snake_case"] = toSnakeCase(value.key);
                valueData["enum_value_kebab_case"] = toKebabCase(value.key);
                if (value.key != enumValueCamel)
                {
                    valueData["enum_value_needs_explicit_value"] = true;
                }
                valueData["last"] =
                    (valueIndex == enumInfo.values.size() - 1);
                enumValues.push_back(valueData);
            }
            enumData["enum_values"] = enumValues;
            enums.push_back(enumData);
        }
        riveFileData["enums"] = enums;

        // Add view models to template data
        std::vector<kainjow::mustache::data> viewmodels;
        for (size_t vmIndex = 0; vmIndex < fileData.viewmodels.size();
             vmIndex++)
        {
            const auto& viewModel = fileData.viewmodels[vmIndex];
            kainjow::mustache::data viewmodelData;
            viewmodelData["view_model_name"] = viewModel.name;
            viewmodelData["view_model_camel_case"] =
                toCamelCase(viewModel.name);
            viewmodelData["view_model_pascal_case"] =
                toPascalCase(viewModel.name);
            viewmodelData["view_model_snake_case"] =
                toSnakeCase(viewModel.name);
            viewmodelData["view_model_kebab_case"] =
                toKebabCase(viewModel.name);
            viewmodelData["last"] =
                (vmIndex == fileData.viewmodels.size() - 1);
            viewmodelData["is_first"] = (vmIndex == 0);

            std::vector<kainjow::mustache::data> properties;
            for (size_t propIndex = 0;
                 propIndex < viewModel.properties.size();
                 propIndex++)
            {
                const auto& property = viewModel.properties[propIndex];
                kainjow::mustache::data propertyData;
                propertyData["property_name"] = property.name;
                propertyData["property_camel_case"] =
                    toCamelCase(property.name);
                propertyData["property_pascal_case"] =
                    toPascalCase(property.name);
                propertyData["property_snake_case"] =
                    toSnakeCase(property.name);
                propertyData["property_kebab_case"] =
                    toKebabCase(property.name);
                propertyData["property_type"] = property.type;

                // Add property type information for the viewmodel template
                kainjow::mustache::data propertyTypeData;
                propertyTypeData.set("is_view_model",
                                       property.type == "viewModel");
                propertyTypeData.set("is_enum", property.type == "enum");
                propertyTypeData.set("is_string", property.type == "string");
                propertyTypeData.set("is_number", property.type == "number");
                propertyTypeData.set("is_integer",
                                       property.type == "integer");
                propertyTypeData.set("is_boolean",
                                       property.type == "boolean");
                propertyTypeData.set("is_color", property.type == "color");
                propertyTypeData.set("is_list", property.type == "list");
                propertyTypeData.set("is_image", property.type == "image" || property.type == "assetImage");
                propertyTypeData.set("is_trigger",
                                       property.type == "trigger");
                propertyTypeData.set("backing_name", property.backingName);
                propertyTypeData.set("backing_camel_case",
                                       toCamelCase(property.backingName));
                propertyTypeData.set("backing_pascal_case",
                                       toPascalCase(property.backingName));
                propertyTypeData.set("backing_snake_case",
                                       toSnakeCase(property.backingName));
                propertyTypeData.set("backing_kebab_case",
                                       toKebabCase(property.backingName));

                // Add default values for properties
                if (!property.defaultValue.empty()) {
                    propertyTypeData.set("default_value", property.defaultValue);

                    if (property.type == "enum") {
                        propertyTypeData.set("enum_default_value", property.defaultValue);
                        propertyTypeData.set("enum_default_value_camel",
                                               toCamelCase(property.defaultValue));
                    }
                }

                propertyData.set("property_type", propertyTypeData);

                propertyData["last"] =
                    (propIndex == viewModel.properties.size() - 1);
                properties.push_back(propertyData);
            }
            viewmodelData["properties"] = properties;
            viewmodels.push_back(viewmodelData);
        }
        riveFileData["view_models"] = viewmodels;

        std::vector<kainjow::mustache::data> assets;
        for (size_t assetIndex = 0; assetIndex < fileData.assets.size();
             assetIndex++)
        {
            const auto& asset = fileData.assets[assetIndex];
            kainjow::mustache::data assetData;
            assetData["asset_name"] = asset.name;
            assetData["asset_camel_case"] = toCamelCase(asset.name);
            assetData["asset_pascal_case"] = toPascalCase(asset.name);
            assetData["asset_snake_case"] = toSnakeCase(asset.name);
            assetData["asset_kebab_case"] = toKebabCase(asset.name);
            assetData["asset_type"] = asset.type;
            assetData["asset_id"] = asset.assetId;
            assetData["asset_cdn_uuid"] = asset.cdnUuid;
            assetData["asset_cdn_base_url"] = asset.cdnBaseUrl;
            assetData["last"] = (assetIndex == fileData.assets.size() - 1);
            assets.push_back(assetData);
        }
        riveFileData["assets"] = assets;

        std::vector<kainjow::mustache::data> artboardList;
        for (size_t artboardIndex = 0;
             artboardIndex < fileData.artboards.size();
             artboardIndex++)
        {
            const auto& artboard = fileData.artboards[artboardIndex];
            kainjow::mustache::data artboardData;
            artboardData["artboard_name"] = artboard.artboardName;
            artboardData["artboard_pascal_case"] =
                artboard.artboardPascalCase;
            artboardData["artboard_camel_case"] = artboard.artboardCameCase;
            artboardData["artboard_snake_case"] = artboard.artboardSnakeCase;
            artboardData["artboard_kebab_case"] = artboard.artboardKebabCase;
            artboardData["last"] =
                (artboardIndex == fileData.artboards.size() - 1);
            
            // Add relationship information
            artboardData["is_default"] = artboard.isDefault;
            artboardData["view_model_id"] = static_cast<int>(artboard.viewModelId);
            artboardData["view_model_name"] = artboard.viewModelName;
            artboardData["has_view_model"] = artboard.hasViewModel;
            artboardData["default_state_machine_name"] = artboard.defaultStateMachineName;
            artboardData["has_default_state_machine"] = artboard.hasDefaultStateMachine;

            std::unordered_set<std::string> usedAnimationNames;
            std::vector<kainjow::mustache::data> animations;
            for (size_t animIndex = 0; animIndex < artboard.animations.size();
                 animIndex++)
            {
                const auto& animation = artboard.animations[animIndex];
                kainjow::mustache::data animData;
                auto uniqueName = makeUnique(animation, usedAnimationNames);
                animData["animation_name"] = animation;
                animData["animation_camel_case"] = toCamelCase(uniqueName);
                animData["animation_pascal_case"] = toPascalCase(uniqueName);
                animData["animation_snake_case"] = toSnakeCase(uniqueName);
                animData["animation_kebab_case"] = toKebabCase(uniqueName);
                animData["last"] =
                    (animIndex == artboard.animations.size() - 1);
                animations.push_back(animData);
            }
            artboardData["animations"] = animations;

            std::unordered_set<std::string> usedStateMachineNames;
            std::vector<kainjow::mustache::data> stateMachines;
            for (size_t smIndex = 0; smIndex < artboard.stateMachines.size();
                 smIndex++)
            {
                const auto& stateMachine = artboard.stateMachines[smIndex];
                kainjow::mustache::data stateMachineData;
                auto uniqueName =
                    makeUnique(stateMachine.first, usedStateMachineNames);
                stateMachineData["state_machine_name"] = stateMachine.first;
                stateMachineData["state_machine_camel_case"] =
                    toCamelCase(uniqueName);
                stateMachineData["state_machine_pascal_case"] =
                    toPascalCase(uniqueName);
                stateMachineData["state_machine_snake_case"] =
                    toSnakeCase(uniqueName);
                stateMachineData["state_machine_kebab_case"] =
                    toKebabCase(uniqueName);
                stateMachineData["last"] =
                    (smIndex == artboard.stateMachines.size() - 1);

                std::unordered_set<std::string> usedInputNames;
                std::vector<kainjow::mustache::data> inputs;
                for (size_t inputIndex = 0;
                     inputIndex < stateMachine.second.size();
                     inputIndex++)
                {
                    const auto& input = stateMachine.second[inputIndex];
                    kainjow::mustache::data inputData;
                    auto uniqueName = makeUnique(input.name, usedInputNames);
                    inputData["input_name"] = input.name;
                    inputData["input_camel_case"] = toCamelCase(uniqueName);
                    inputData["input_pascal_case"] = toPascalCase(uniqueName);
                    inputData["input_snake_case"] = toSnakeCase(uniqueName);
                    inputData["input_kebab_case"] = toKebabCase(uniqueName);
                    inputData["input_type"] = input.type;
                    inputData["input_default_value"] = input.defaultValue;
                    inputData["last"] =
                        (inputIndex == stateMachine.second.size() - 1);
                    inputs.push_back(inputData);
                }
                stateMachineData["inputs"] = inputs;

                stateMachines.push_back(stateMachineData);
            }
            artboardData["state_machines"] = stateMachines;

            std::unordered_set<std::string> usedTextValueRunNames;
            std::vector<kainjow::mustache::data> textValueRuns;
            for (size_t tvrIndex = 0;
                 tvrIndex < artboard.textValueRuns.size();
                 tvrIndex++)
            {
                const auto& tvr = artboard.textValueRuns[tvrIndex];
                kainjow::mustache::data tvrData;
                auto uniqueName = makeUnique(tvr.name, usedTextValueRunNames);
                tvrData["text_value_run_name"] = tvr.name;
                tvrData["text_value_run_camel_case"] =
                    toCamelCase(uniqueName);
                tvrData["text_value_run_pascal_case"] =
                    toPascalCase(uniqueName);
                tvrData["text_value_run_snake_case"] =
                    toSnakeCase(uniqueName);
                tvrData["text_value_run_kebab_case"] =
                    toKebabCase(uniqueName);
                tvrData["text_value_run_default"] = tvr.defaultValue;
                tvrData["text_value_run_default_sanitized"] =
                    sanitizeString(tvr.defaultValue);
                tvrData["last"] =
                    (tvrIndex == artboard.textValueRuns.size() - 1);
                textValueRuns.push_back(tvrData);
            }
            artboardData["text_value_runs"] = textValueRuns;

            std::vector<kainjow::mustache::data> nestedTextValueRuns;
            for (size_t ntvrIndex = 0;
                 ntvrIndex < artboard.nestedTextValueRuns.size();
                 ntvrIndex++)
            {
                const auto& ntvr = artboard.nestedTextValueRuns[ntvrIndex];
                kainjow::mustache::data ntvrData;
                ntvrData["nested_text_value_run_name"] = ntvr.name;
                ntvrData["nested_text_value_run_path"] = ntvr.path;
                ntvrData["last"] =
                    (ntvrIndex == artboard.nestedTextValueRuns.size() - 1);
                nestedTextValueRuns.push_back(ntvrData);
            }

            artboardData["nested_text_value_runs"] = nestedTextValueRuns;

            artboardList.push_back(artboardData);
        }

        riveFileData["artboards"] = artboardList;

        // Add count flags for conditional generation
        riveFileData["artboard_count"] = fileData.artboards.size();
        riveFileData["has_multiple_artboards"] = fileData.artboards.size() > 1;

        // Count total animations and state machines across all artboards
        size_t totalAnimations = 0;
        size_t totalStateMachines = 0;
        for (const auto& artboard : fileData.artboards) {
            totalAnimations += artboard.animations.size();
            totalStateMachines += artboard.stateMachines.size();
        }
        riveFileData["total_animation_count"] = totalAnimations;
        riveFileData["has_multiple_animations"] = totalAnimations > 1;
        riveFileData["total_state_machine_count"] = totalStateMachines;
        riveFileData["has_state_machines"] = totalStateMachines > 0;
        riveFileData["has_multiple_state_machines"] = totalStateMachines > 1;

        // Add metadata flag - show metadata if there are multiples of any type
        bool hasMetadata = fileData.artboards.size() > 1 || totalAnimations > 1 || totalStateMachines > 1;
        riveFileData["has_metadata"] = hasMetadata;

        // Add view model existence flag
        bool hasViewModel = !fileData.viewmodels.empty();
        riveFileData["has_view_model"] = hasViewModel;

        // Add type-safe switching flag - show type-safe methods only if there will be actual methods
        // Methods are shown when:
        // - switchArtboard: has_multiple_artboards
        // - playAnimation: !has_view_model && !has_state_machines && has_multiple_animations
        // - switchStateMachine: has_multiple_state_machines
        bool hasTypeSafeSwitching = fileData.artboards.size() > 1 ||
                                     (!hasViewModel && totalStateMachines == 0 && totalAnimations > 1) ||
                                     totalStateMachines > 1;
        riveFileData["has_type_safe_switching"] = hasTypeSafeSwitching;

        riveFileList.push_back(riveFileData);
    }

    templateData["generated_file_name"] = generatedFileName;
    templateData["riv_files"] = riveFileList;

    // Render template based on selected engine
    std::string result;
    if (templateEngine == TemplateEngine::Mustache)
    {
        kainjow::mustache::mustache tmpl(templateStr);
        result = tmpl.render(templateData);
        std::cout << "Using Mustache template engine" << std::endl;
    }
    else if (templateEngine == TemplateEngine::Inja)
    {
        try
        {
            // Build inja-compatible JSON directly from riveFileDataList
            nlohmann::json injaData = buildInjaData(riveFileDataList);

            inja::Environment env;

            // Configure inja settings for optimal performance
            env.set_trim_blocks(true);
            env.set_lstrip_blocks(true);
            env.set_html_autoescape(false);  // Disable HTML escaping (not needed for Swift code)
            env.set_throw_at_missing_includes(false);  // No includes in our templates

            // Pre-parse template for better performance
            auto tmpl = env.parse(templateStr);
            result = env.render(tmpl, injaData);
            std::cout << "Using Inja template engine" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: Inja rendering failed: " << e.what() << std::endl;
            return 1;
        }
    }
    else
    {
        std::cerr << "Error: Unknown template engine" << std::endl;
        return 1;
    }

    std::cout << "Rive: output_file_path = " << outputFilePath << std::endl;

    std::filesystem::path output_path(outputFilePath);

    // If only a filename is provided, use the current directory
    if (output_path.is_relative() && output_path.parent_path().empty())
    {
        output_path = std::filesystem::current_path() / output_path;
    }

    // Create directories if they don't exist (this won't do anything if it's
    // just a filename)
    std::filesystem::create_directories(output_path.parent_path());

    std::ofstream output_file(output_path);
    if (!output_file.is_open())
    {
        std::cerr << "Error: Unable to open output file: " << output_path
                  << std::endl;
        return 1;
    }
    output_file << result;
    output_file.close();

    std::cout << "File generated successfully: " << output_path << std::endl;

    return 0;
}
