#include "rive_file_parser.h"
#include "byte_stats.h"
#include "string_utils.h"

#include "rive/animation/linear_animation_instance.hpp"
#include "rive/animation/state_machine.hpp"
#include "rive/animation/state_machine_bool.hpp"
#include "rive/animation/state_machine_input_instance.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "rive/animation/state_machine_number.hpp"
#include "rive/animation/state_machine_trigger.hpp"
#include "rive/assets/audio_asset.hpp"
#include "rive/assets/font_asset.hpp"
#include "rive/assets/image_asset.hpp"
#include "rive/assets/script_asset.hpp"
#include "rive/assets/shader_asset.hpp"
#include "rive/bones/bone.hpp"
#include "rive/generated/animation/state_machine_bool_base.hpp"
#include "rive/generated/animation/state_machine_number_base.hpp"
#include "rive/generated/animation/state_machine_trigger_base.hpp"
#include "rive/generated/constraints/constraint_base.hpp"
#include "rive/nested_artboard.hpp"
#include "rive/shapes/clipping_shape.hpp"
#include "rive/shapes/image.hpp"
#include "rive/shapes/mesh.hpp"
#include "rive/shapes/mesh_vertex.hpp"
#include "rive/shapes/paint/feather.hpp"
#include "rive/shapes/paint/fill.hpp"
#include "rive/shapes/paint/linear_gradient.hpp"
#include "rive/shapes/paint/radial_gradient.hpp"
#include "rive/shapes/paint/stroke.hpp"
#include "rive/shapes/paint/trim_path.hpp"
#include "rive/shapes/path.hpp"
#include "rive/shapes/path_vertex.hpp"
#include "rive/shapes/shape.hpp"
#include "rive/text/text_value_run.hpp"
#include "rive/viewmodel/data_enum.hpp"
#include "rive/viewmodel/data_enum_value.hpp"
#include "rive/viewmodel/runtime/viewmodel_runtime.hpp"
#include "rive/viewmodel/viewmodel_property_enum.hpp"

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace
{
EnumInfo makeEnumInfo(rive::DataEnum* dataEnum)
{
    EnumInfo enumInfo;
    enumInfo.name = dataEnum->enumName();
    const auto& values = dataEnum->values();
    for (const auto* value : values)
    {
        enumInfo.values.push_back({value->key()});
    }
    return enumInfo;
}

void upsertEnumInfo(std::vector<EnumInfo>& enums, rive::DataEnum* dataEnum)
{
    auto enumInfo = makeEnumInfo(dataEnum);
    for (auto& existingEnum : enums)
    {
        if (existingEnum.name == enumInfo.name)
        {
            std::unordered_set<std::string> existingValues;
            for (const auto& value : existingEnum.values)
            {
                existingValues.insert(value.key);
            }
            for (const auto& value : enumInfo.values)
            {
                if (existingValues.insert(value.key).second)
                {
                    existingEnum.values.push_back(value);
                }
            }
            return;
        }
    }
    enums.push_back(enumInfo);
}
} // namespace

// ---------------------------------------------------------------------------
// File discovery
// ---------------------------------------------------------------------------

std::vector<std::string> findRiveFiles(const std::string& path)
{
    std::vector<std::string> riveFiles;

    if (std::filesystem::is_regular_file(path))
    {
        if (std::filesystem::path(path).extension() == ".riv")
        {
            riveFiles.push_back(path);
        }
        return riveFiles;
    }

    if (std::filesystem::is_directory(path))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".riv")
            {
                riveFiles.push_back(entry.path().string());
            }
        }
    }

    return riveFiles;
}

// ---------------------------------------------------------------------------
// File loading
// ---------------------------------------------------------------------------

rive::rcp<rive::File> openFile(const char name[], rive::Factory& factory, rive::FileAssetLoader* loader)
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
        fclose(f);
        return nullptr;
    }

    fclose(f);
#ifdef WITH_RIVE_TOOLS
    rive::ImportResult stripResult = rive::ImportResult::malformed;
    auto strippedBytes =
        rive::File::stripAssets(bytes,
                                {rive::ScriptAsset::typeKey,
                                 rive::ShaderAsset::typeKey},
                                &stripResult);
    if (stripResult == rive::ImportResult::success)
    {
        return rive::File::import(strippedBytes, &factory, nullptr, loader);
    }
#endif
    return rive::File::import(bytes, &factory, nullptr, loader);
}

// ---------------------------------------------------------------------------
// Filtering
// ---------------------------------------------------------------------------

static bool shouldIncludeElement(const std::string& name, bool ignorePrivate)
{
    if (!ignorePrivate)
    {
        return true;
    }

    if (!name.empty() && name[0] == '_')
    {
        return false;
    }

    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    if (lowerName.rfind("internal", 0) == 0 || lowerName.rfind("private", 0) == 0)
    {
        return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Artboard helpers
// ---------------------------------------------------------------------------

template <typename T = rive::Component>
static void findAll(std::vector<T*>& results, rive::ArtboardInstance* artboard)
{
    for (auto object : artboard->objects())
    {
        if (object != nullptr && object->is<T>())
        {
            results.push_back(static_cast<T*>(object));
        }
    }
}

static std::vector<std::string> getAnimationsFromArtboard(rive::ArtboardInstance* artboard,
                                                            bool ignorePrivate = false)
{
    std::vector<std::string> animations;
    auto animationCount = artboard->animationCount();
    for (int i = 0; i < animationCount; i++)
    {
        auto animation = artboard->animationAt(i);
        std::string animationName = animation->name();

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
        auto stateMachine = artboard->stateMachine(i);
        if (stateMachine == nullptr)
        {
            continue;
        }
        std::string stateMachineName = stateMachine->name();

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

            switch (input->coreType())
            {
                case rive::StateMachineNumberBase::typeKey:
                {
                    auto smiNumberInput = static_cast<const rive::StateMachineNumber*>(input);
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
                    auto smiBoolInput = static_cast<const rive::StateMachineBool*>(input);
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
            textValueRunsInfo.push_back({textValueRun->name(), textValueRun->text()});
        }
    }
    return textValueRunsInfo;
}

static std::vector<NestedTextValueRunInfo> getNestedTextValueRunPathsFromArtboard(
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
            nestedTextValueRunsInfo.push_back({textRun.name, currentPath});
        }
    }

    for (int i = 0; i < count; i++)
    {
        auto nested = artboard->nestedArtboards()[i];
        auto nestedName = nested->name();
        if (!nestedName.empty())
        {
            std::string newPath = currentPath.empty()
                                       ? nested->name()
                                       : currentPath + "/" + nested->name();

            auto nestedArtboardInst = nested->artboardInstance();
            if (nestedArtboardInst)
            {
                auto nestedResults =
                    getNestedTextValueRunPathsFromArtboard(nestedArtboardInst, newPath);
                nestedTextValueRunsInfo.insert(nestedTextValueRunsInfo.end(),
                                               nestedResults.begin(),
                                               nestedResults.end());
            }
        }
    }

    return nestedTextValueRunsInfo;
}

static void accumulateArtboardStats(
    rive::ArtboardInstance* artboard,
    const std::unordered_map<const rive::ImageAsset*, AssetInfo>& assetsByImage,
    ArtboardStats& stats,
    std::unordered_set<std::string>& seenAssetIds,
    std::unordered_set<const rive::ArtboardInstance*>& seenArtboards)
{
    if (artboard == nullptr || !seenArtboards.insert(artboard).second)
    {
        return;
    }

    stats.objectCount += artboard->objects().size();
    stats.nestedArtboardCount += artboard->nestedArtboards().size();

    for (auto object : artboard->objects())
    {
        if (object == nullptr)
        {
            continue;
        }

        if (object->is<rive::Image>())
        {
            stats.imageCount++;
            auto imageAsset = object->as<rive::Image>()->imageAsset();
            auto assetIt = assetsByImage.find(imageAsset);
            auto assetId = assetIt == assetsByImage.end() ? std::to_string(object->as<rive::Image>()->assetId())
                                                          : assetIt->second.assetId;
            if (seenAssetIds.insert(assetId).second)
            {
                if (assetIt != assetsByImage.end())
                {
                    stats.referencedDecodedRGBAByteSize += assetIt->second.decodedRGBAByteSize;
                    stats.referencedAssets.push_back({assetIt->second.name,
                                                       assetId,
                                                       assetIt->second.decodedRGBAByteSize});
                }
                else
                {
                    stats.referencedAssets.push_back({"", assetId, 0});
                }
            }
        }
        if (object->is<rive::Shape>())
        {
            stats.shapeCount++;
        }
        if (object->is<rive::Path>())
        {
            stats.pathCount++;
        }
        if (object->is<rive::PathVertex>())
        {
            stats.pathVertexCount++;
        }
        if (object->is<rive::Mesh>())
        {
            stats.meshCount++;
        }
        if (object->is<rive::MeshVertex>())
        {
            stats.meshVertexCount++;
        }
        if (object->is<rive::ClippingShape>())
        {
            stats.clippingShapeCount++;
        }
        if (object->is<rive::Fill>())
        {
            stats.fillCount++;
        }
        if (object->is<rive::Stroke>())
        {
            stats.strokeCount++;
        }
        if (object->is<rive::LinearGradient>() || object->is<rive::RadialGradient>())
        {
            stats.gradientCount++;
        }
        if (object->is<rive::TrimPath>())
        {
            stats.trimPathCount++;
        }
        if (object->is<rive::Feather>())
        {
            stats.featherCount++;
        }
        if (object->is<rive::Bone>())
        {
            stats.boneCount++;
        }
        if (object->is<rive::ConstraintBase>())
        {
            stats.constraintCount++;
        }
    }
    for (auto nestedArtboard : artboard->nestedArtboards())
    {
        accumulateArtboardStats(nestedArtboard->artboardInstance(),
                                assetsByImage,
                                stats,
                                seenAssetIds,
                                seenArtboards);
    }
}

static ArtboardStats getArtboardStatsFromArtboard(
    rive::ArtboardInstance* artboard,
    const std::unordered_map<const rive::ImageAsset*, AssetInfo>& assetsByImage)
{
    ArtboardStats stats;
    std::unordered_set<std::string> seenAssetIds;
    std::unordered_set<const rive::ArtboardInstance*> seenArtboards;
    accumulateArtboardStats(artboard, assetsByImage, stats, seenAssetIds, seenArtboards);

    return stats;
}

// ---------------------------------------------------------------------------
// Asset extraction
// ---------------------------------------------------------------------------

static std::string dataTypeToString(rive::DataType type)
{
    switch (type)
    {
        case rive::DataType::none:            return "none";
        case rive::DataType::string:          return "string";
        case rive::DataType::number:          return "number";
        case rive::DataType::boolean:         return "boolean";
        case rive::DataType::color:           return "color";
        case rive::DataType::list:            return "list";
        case rive::DataType::enumType:        return "enum";
        case rive::DataType::trigger:         return "trigger";
        case rive::DataType::viewModel:       return "viewModel";
        case rive::DataType::integer:         return "integer";
        case rive::DataType::symbolListIndex: return "symbolListIndex";
        case rive::DataType::assetImage:      return "assetImage";
        case rive::DataType::artboard:        return "artboard";
        default:                              return "unknown";
    }
}

static std::vector<AssetInfo> getAssetsFromFile(
    rive::File* file,
    const std::unordered_map<std::string, EmbeddedImageInfo>& embeddedImages)
{
    std::vector<AssetInfo> assetsInfo;
    std::unordered_set<std::string> usedAssetNames;

    auto assets = file->assets();
    for (auto asset : assets)
    {
        std::string assetType;
        switch (asset->coreType())
        {
            case rive::ImageAsset::typeKey: assetType = "image"; break;
            case rive::FontAsset::typeKey:  assetType = "font";  break;
            case rive::AudioAsset::typeKey: assetType = "audio"; break;
            default:                        assetType = "unknown"; break;
        }

        auto assetName = asset->name();
        auto uniqueAssetName = makeUnique(assetName, usedAssetNames);

        // Look up embedded info by asset name. Only embedded images appear in the map
        // (inBandBytes.size() > 0 in loadContents). Referenced and CDN images are absent,
        // so embeddedByteSize stays 0 and fileExtension falls back to asset->fileExtension().
        // The detected extension from magic bytes takes precedence over the metadata extension,
        // which reflects the source filename and may not match the actual embedded format.
        size_t embeddedByteSize = 0;
        std::string fileExtension = asset->fileExtension();
        if (assetType == "image")
        {
            auto it = embeddedImages.find(assetName);
            if (it != embeddedImages.end())
            {
                embeddedByteSize = it->second.byteSize;
                if (!it->second.detectedExtension.empty())
                    fileExtension = it->second.detectedExtension;
                auto width = it->second.width;
                auto height = it->second.height;
                assetsInfo.push_back(AssetInfo{uniqueAssetName,
                                               assetName,
                                               assetType,
                                               fileExtension,
                                               std::to_string(asset->assetId()),
                                               asset->cdnUuidStr(),
                                               asset->cdnBaseUrl(),
                                               embeddedByteSize,
                                               width,
                                               height,
                                               static_cast<size_t>(width) * height * 4});
                continue;
            }
        }

        assetsInfo.push_back(AssetInfo{uniqueAssetName,
                                       assetName,
                                       assetType,
                                       fileExtension,
                                       std::to_string(asset->assetId()),
                                       asset->cdnUuidStr(),
                                       asset->cdnBaseUrl(),
                                       embeddedByteSize,
                                       0,
                                       0,
                                       0});
    }
    return assetsInfo;
}

// ---------------------------------------------------------------------------
// processRiveFile
// ---------------------------------------------------------------------------

std::optional<RiveFileData> processRiveFile(const std::string& riveFilePath, bool ignorePrivate)
{
    if (std::filesystem::is_empty(riveFilePath))
    {
        std::cerr << "Error: Rive file is empty: " << riveFilePath << std::endl;
        return std::nullopt;
    }

    rive::NoOpFactory factory;
    LintingAssetLoader loader;
    auto riveFile = openFile(riveFilePath.c_str(), factory, &loader);
    if (!riveFile)
    {
        std::cerr << "Error: Failed to parse Rive file: " << riveFilePath << std::endl;
        return std::nullopt;
    }

    std::filesystem::path path(riveFilePath);
    std::string fileNameWithoutExtension = path.stem().string();
    std::vector<AssetInfo> assets = getAssetsFromFile(riveFile.get(), loader.embeddedImages);
    std::unordered_map<const rive::ImageAsset*, AssetInfo> assetsByImage;
    auto riveAssets = riveFile->assets();
    for (size_t i = 0; i < riveAssets.size() && i < assets.size(); i++)
    {
        auto riveAsset = riveAssets[i];
        if (riveAsset != nullptr && riveAsset->is<rive::ImageAsset>())
        {
            assetsByImage.emplace(riveAsset->as<rive::ImageAsset>(), assets[i]);
        }
    }

    RiveFileData fileData;
    fileData.rivOriginalFileName = fileNameWithoutExtension;
    fileData.rivPascalCase = toPascalCase(fileNameWithoutExtension);
    fileData.rivCameCase = toCamelCase(fileNameWithoutExtension);
    fileData.riveSnakeCase = toSnakeCase(fileNameWithoutExtension);
    fileData.rivKebabCase = toKebabCase(fileNameWithoutExtension);
    fileData.assets = assets;
    fileData.byteStats = analyzeRiveByteStats(riveFilePath);

    // Process enums (merge by name — riv files can reference the same enum multiple times,
    // and later copies may include newer values).
    const auto& fileEnums = riveFile->enums();
    for (auto* dataEnum : fileEnums)
    {
        if (dataEnum)
        {
            upsertEnumInfo(fileData.enums, dataEnum);
        }
    }

    std::unordered_set<std::string> usedEnumNames;
    std::unordered_map<uint32_t, std::string> viewModelIdToName;

    // Process view models
    for (size_t i = 0; i < riveFile->viewModelCount(); i++)
    {
        auto viewModel = riveFile->viewModelByIndex(i);
        if (viewModel)
        {
            std::cout << "Processing ViewModel " << i << ": " << viewModel->name() << std::endl;

            viewModelIdToName[i] = viewModel->name();

            if (!shouldIncludeElement(viewModel->name(), ignorePrivate))
            {
                std::cout << "  Skipping ViewModel (private): " << viewModel->name() << std::endl;
                continue;
            }

            std::cout << "  Including ViewModel: " << viewModel->name() << std::endl;
            ViewModelInfo viewModelInfo;
            viewModelInfo.name = viewModel->name();
            std::cout << "  Getting properties..." << std::endl;
            auto propertiesData = viewModel->properties();
            std::cout << "  Found " << propertiesData.size() << " properties" << std::endl;

            for (const auto& property : propertiesData)
            {
                if (!shouldIncludeElement(property.name, ignorePrivate))
                {
                    continue;
                }

                if (property.type == rive::DataType::viewModel)
                {
                    auto vmInstance = viewModel->createInstance();
                    if (!vmInstance)
                    {
                        std::cerr << "Warning: Failed to create instance for ViewModel: "
                                  << viewModel->name() << std::endl;
                        continue;
                    }

                    auto nestedViewModel = vmInstance->propertyViewModel(property.name);
                    if (!nestedViewModel)
                    {
                        std::cerr << "Warning: Failed to get property ViewModel: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto nestedInstance = nestedViewModel->instance();
                    if (!nestedInstance)
                    {
                        std::cerr << "Warning: Failed to get nested ViewModel instance for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto vm = nestedInstance->viewModel();
                    if (!vm)
                    {
                        std::cerr << "Warning: Failed to get nested ViewModel for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    if (!shouldIncludeElement(vm->name(), ignorePrivate))
                    {
                        continue;
                    }

                    viewModelInfo.properties.push_back(
                        {property.name, dataTypeToString(property.type), vm->name(), ""});
                }
                else if (property.type == rive::DataType::enumType)
                {
                    auto vmi = riveFile->createViewModelInstance(viewModel->name());
                    if (!vmi)
                    {
                        std::cerr << "Warning: Failed to create ViewModel instance for: "
                                  << viewModel->name() << std::endl;
                        continue;
                    }

                    auto propertyValue = vmi->propertyValue(property.name);
                    if (!propertyValue)
                    {
                        std::cerr << "Warning: Failed to get property value for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto enum_instance = static_cast<rive::ViewModelInstanceEnum*>(propertyValue);
                    if (!enum_instance)
                    {
                        std::cerr << "Warning: Failed to cast to ViewModelInstanceEnum for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto vmProperty = enum_instance->viewModelProperty();
                    if (!vmProperty)
                    {
                        std::cerr << "Warning: Failed to get viewModelProperty for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto enumProperty = vmProperty->as<rive::ViewModelPropertyEnum>();
                    if (!enumProperty)
                    {
                        std::cerr << "Warning: Failed to cast to ViewModelPropertyEnum for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto dataEnum = enumProperty->dataEnum();
                    if (!dataEnum)
                    {
                        std::cerr << "Warning: Failed to get dataEnum for: "
                                  << property.name << std::endl;
                        continue;
                    }

                    auto enumName = dataEnum->enumName();
                    usedEnumNames.insert(enumName);
                    upsertEnumInfo(fileData.enums, dataEnum);

                    uint32_t defaultIndex = enum_instance->propertyValue();
                    std::string defaultValue = "";
                    if (defaultIndex < dataEnum->values().size())
                    {
                        defaultValue = dataEnum->values()[defaultIndex]->key();
                    }

                    viewModelInfo.properties.push_back(
                        {property.name, dataTypeToString(property.type), enumName, defaultValue});
                }
                else
                {
                    auto vmi = riveFile->createViewModelInstance(viewModel->name());
                    if (!vmi)
                    {
                        std::cerr << "Warning: Failed to create ViewModel instance for: "
                                  << viewModel->name() << std::endl;
                        continue;
                    }

                    std::string defaultValue = "";

                    if (property.type == rive::DataType::boolean)
                    {
                        auto pv = vmi->propertyValue(property.name);
                        if (pv)
                        {
                            auto b = static_cast<rive::ViewModelInstanceBoolean*>(pv);
                            if (b) defaultValue = b->propertyValue() ? "true" : "false";
                        }
                    }
                    else if (property.type == rive::DataType::number)
                    {
                        auto pv = vmi->propertyValue(property.name);
                        if (pv)
                        {
                            auto n = static_cast<rive::ViewModelInstanceNumber*>(pv);
                            if (n) defaultValue = std::to_string(n->propertyValue());
                        }
                    }
                    else if (property.type == rive::DataType::string)
                    {
                        auto pv = vmi->propertyValue(property.name);
                        if (pv)
                        {
                            auto s = static_cast<rive::ViewModelInstanceString*>(pv);
                            if (s) defaultValue = s->propertyValue();
                        }
                    }
                    else if (property.type == rive::DataType::color)
                    {
                        auto pv = vmi->propertyValue(property.name);
                        if (pv)
                        {
                            auto c = static_cast<rive::ViewModelInstanceColor*>(pv);
                            if (c)
                            {
                                std::stringstream ss;
                                ss << "0x" << std::hex << std::uppercase << std::setfill('0')
                                   << std::setw(8) << static_cast<unsigned int>(c->propertyValue());
                                defaultValue = ss.str();
                            }
                        }
                    }
                    // assetImage, trigger and other types don't have extractable default values

                    viewModelInfo.properties.push_back(
                        {property.name, dataTypeToString(property.type), "", defaultValue});
                }
            }

            fileData.viewmodels.push_back(viewModelInfo);
            std::cout << "  Finished processing ViewModel: " << viewModel->name() << std::endl;
        }
    }

    std::cout << "Finished processing all ViewModels. Total included: "
              << fileData.viewmodels.size() << std::endl;

    // Filter enums to only those used by non-filtered ViewModels
    if (ignorePrivate && !fileData.viewmodels.empty())
    {
        std::vector<EnumInfo> filteredEnums;
        for (const auto& enumInfo : fileData.enums)
        {
            if (usedEnumNames.count(enumInfo.name) > 0)
            {
                filteredEnums.push_back(enumInfo);
            }
        }
        fileData.enums = filteredEnums;
    }

    std::cout << "Extracting default relationship chain..." << std::endl;

    auto defaultArtboard = riveFile->artboard();
    fileData.hasDefaults = (defaultArtboard != nullptr);

    if (fileData.hasDefaults)
    {
        fileData.defaultArtboardName = defaultArtboard->name();
        std::cout << "  Default artboard: " << fileData.defaultArtboardName << std::endl;

        auto defaultStateMachineIndex = defaultArtboard->defaultStateMachineIndex();
        if (defaultStateMachineIndex >= 0)
        {
            auto defaultStateMachine = defaultArtboard->stateMachine(defaultStateMachineIndex);
            if (defaultStateMachine)
            {
                fileData.defaultStateMachineName = defaultStateMachine->name();
                std::cout << "  Default state machine: "
                          << fileData.defaultStateMachineName << std::endl;
            }
        }

        auto defaultViewModelRuntime = riveFile->defaultArtboardViewModel(defaultArtboard);
        if (defaultViewModelRuntime)
        {
            fileData.defaultViewModelName = defaultViewModelRuntime->name();
            std::cout << "  Default ViewModel: " << fileData.defaultViewModelName << std::endl;
        }
    }

    std::cout << "Processing artboards..." << std::endl;

    std::unordered_set<std::string> usedArtboardNames;
    auto artboardCount = riveFile->artboardCount();
    std::cout << "Total artboard count: " << artboardCount << std::endl;

    for (int i = 0; i < artboardCount; i++)
    {
        auto artboard = riveFile->artboardAt(i);
        std::string artboardName = artboard->name();
        std::cout << "Processing artboard " << i << ": " << artboardName << std::endl;

        if (!shouldIncludeElement(artboardName, ignorePrivate))
        {
            std::cout << "  Skipping artboard (private): " << artboardName << std::endl;
            continue;
        }

        std::string artboardPascalCase = toPascalCase(artboardName);
        std::string artboardCameCase = toCamelCase(artboardName);
        std::string artboardSnakeCase = toSnakeCase(artboardName);
        std::string artboardKebabCase = toKebabCase(artboardName);

        artboardCameCase = makeUnique(artboardCameCase, usedArtboardNames);

        std::cout << "  Getting animations..." << std::endl;
        auto animations = getAnimationsFromArtboard(artboard.get(), ignorePrivate);
        std::cout << "  Getting state machines..." << std::endl;
        auto stateMachines = getStateMachinesFromArtboard(artboard.get(), ignorePrivate);
        std::cout << "  Getting text value runs..." << std::endl;
        auto textValueRuns = getTextValueRunsFromArtboard(artboard.get());
        std::cout << "  Getting nested text value runs..." << std::endl;
        auto nestedTextValueRuns = getNestedTextValueRunPathsFromArtboard(artboard.get());
        auto stats = getArtboardStatsFromArtboard(artboard.get(), assetsByImage);
        std::cout << "  Done getting artboard data" << std::endl;

        bool isDefault = (i == 0);
        uint32_t artboardViewModelId = artboard->viewModelId();

        std::string viewModelName = "";
        bool hasViewModel = false;
        auto vmIt = viewModelIdToName.find(artboardViewModelId);
        if (vmIt != viewModelIdToName.end())
        {
            viewModelName = vmIt->second;
            for (const auto& vm : fileData.viewmodels)
            {
                if (vm.name == viewModelName)
                {
                    hasViewModel = true;
                    break;
                }
            }
        }

        std::string defaultStateMachineName = "";
        bool hasDefaultStateMachine = false;
        auto defaultStateMachineIndex = artboard->defaultStateMachineIndex();
        if (defaultStateMachineIndex >= 0)
        {
            auto defaultStateMachine = artboard->stateMachine(defaultStateMachineIndex);
            if (defaultStateMachine)
            {
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
                                       hasDefaultStateMachine,
                                       stats});
    }

    return fileData;
}
