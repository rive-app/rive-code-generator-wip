#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "CLIUTILS/CLI11.hpp"
#include "default_template.h"
#include "kainjow/mustache.hpp"
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
};

struct RiveFileData
{
    std::string rivPascalCase;
    std::string rivCameCase;
    std::string riveSnakeCase;
    std::string rivKebabCase;
    std::vector<ArtboardData> artboards;
    std::vector<AssetInfo> assets;
    std::vector<EnumInfo> enums;
    std::vector<ViewModelInfo> viewmodels;
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

static std::unique_ptr<rive::File> openFile(const char name[])
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

static std::vector<std::string> getAnimationsFromArtboard(
    rive::ArtboardInstance* artboard)
{
    std::vector<std::string> animations;
    auto animationCount = artboard->animationCount();
    for (int i = 0; i < animationCount; i++)
    {
        auto animation = artboard->animationAt(i);
        animations.push_back(animation->name());
    }
    return animations;
}

static std::vector<std::pair<std::string, std::vector<InputInfo>>>
getStateMachinesFromArtboard(rive::ArtboardInstance* artboard)
{
    std::vector<std::pair<std::string, std::vector<InputInfo>>> stateMachines;
    auto stateMachineCount = artboard->stateMachineCount();
    for (int i = 0; i < stateMachineCount; i++)
    {
        auto stateMachine = artboard->stateMachineAt(i);
        std::string stateMachineName = stateMachine->name();

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

static std::optional<RiveFileData> processRiveFile(const std::string& riveFilePath)
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

    // Process view models
    for (size_t i = 0; i < riveFile->viewModelCount(); i++)
    {
        auto viewModel = riveFile->viewModelByIndex(i);
        if (viewModel)
        {
            ViewModelInfo viewModelInfo;
            viewModelInfo.name = viewModel->name();
            auto propertiesData = viewModel->properties();
            for (const auto& property : propertiesData)
            {
                if (property.type == rive::DataType::viewModel)
                {
                    // TODO: this is a hack
                    auto nestedViewModel =
                        viewModel->createInstance()->propertyViewModel(
                            property.name);
                    auto vm = nestedViewModel->instance()->viewModel();
                    viewModelInfo.properties.push_back(
                        {property.name,
                         dataTypeToString(property.type),
                         vm->name()});
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
                    viewModelInfo.properties.push_back(
                        {property.name,
                         dataTypeToString(property.type),
                         enumName});
                }
                else
                {
                    viewModelInfo.properties.push_back(
                        {property.name, dataTypeToString(property.type)});
                }
            }
            fileData.viewmodels.push_back(viewModelInfo);
        }
    }

    std::unordered_set<std::string> usedArtboardNames;

    auto artboardCount = riveFile->artboardCount();
    for (int i = 0; i < artboardCount; i++)
    {
        auto artboard = riveFile->artboardAt(i);
        std::string artboardName = artboard->name();

        std::string artboardPascalCase = toPascalCase(artboardName);
        std::string artboardCameCase = toCamelCase(artboardName);
        std::string artboardSnakeCase = toSnakeCase(artboardName);
        std::string artboardKebabCase = toKebabCase(artboardName);

        // Ensure unique artboard variable names
        artboardCameCase =
            makeUnique(artboardCameCase, usedArtboardNames);

        std::vector<std::string> animations =
            getAnimationsFromArtboard(artboard.get());
        std::vector<std::pair<std::string, std::vector<InputInfo>>>
            stateMachines = getStateMachinesFromArtboard(artboard.get());
        std::vector<TextValueRunInfo> textValueRuns =
            getTextValueRunsFromArtboard(artboard.get());
        std::vector<NestedTextValueRunInfo> nestedTextValueRuns =
            getNestedTextValueRunPathsFromArtboard(artboard.get());

        fileData.artboards.push_back({artboardName,
                                       artboardPascalCase,
                                       artboardCameCase,
                                       artboardSnakeCase,
                                       artboardKebabCase,
                                       animations,
                                       stateMachines,
                                       textValueRuns,
                                       nestedTextValueRuns});
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

int main(int argc, char* argv[])
{
    CLI::App app{"Rive Code Generator"};

    std::string inputPath;
    std::string outputFilePath;
    std::string templatePath;
    Language language = Language::Dart; // Default to Dart

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
        auto result = processRiveFile(riv_file);
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
        riveFileData["riv_pascal_case"] = fileData.rivPascalCase;
        riveFileData["riv_camel_case"] = fileData.rivCameCase;
        riveFileData["riv_snake_case"] = fileData.riveSnakeCase;
        riveFileData["riv_kebab_case"] = fileData.rivKebabCase;
        riveFileData["last"] = (fileIndex == riveFileDataList.size() - 1);

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
                valueData["enum_value_key"] = value.key;
                valueData["enum_value_camel_case"] = toCamelCase(value.key);
                valueData["enum_value_pascal_case"] = toPascalCase(value.key);
                valueData["enum_value_snake_case"] = toSnakeCase(value.key);
                valueData["enum_value_kebab_case"] = toKebabCase(value.key);
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
        riveFileList.push_back(riveFileData);
    }

    templateData["generated_file_name"] = generatedFileName;
    templateData["riv_files"] = riveFileList;

    kainjow::mustache::mustache tmpl(templateStr);
    std::string result = tmpl.render(templateData);

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