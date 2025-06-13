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

const std::string generated_file_name = "rive_generated";

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
    std::string default_value;
};

struct TextValueRunInfo
{
    std::string name;
    std::string default_value;
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
    std::string file_extension;
    std::string asset_id;
    std::string cdn_uuid;
    std::string cdn_base_url;
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
    std::string backing_name;
};

struct ViewModelInfo
{
    std::string name;
    std::vector<PropertyInfo> properties;
};

struct ArtboardData
{
    std::string artboard_name;
    std::string artboard_pascal_case;
    std::string artboard_camel_case;
    std::string artboard_snake_case;
    std::string artboard_kebab_case;
    std::vector<std::string> animations;
    std::vector<std::pair<std::string, std::vector<InputInfo>>> state_machines;
    std::vector<TextValueRunInfo> text_value_runs;
    std::vector<NestedTextValueRunInfo> nested_text_value_runs;
};

struct RiveFileData
{
    std::string riv_pascal_case;
    std::string riv_camel_case;
    std::string riv_snake_case;
    std::string riv_kebab_case;
    std::vector<ArtboardData> artboards;
    std::vector<AssetInfo> assets;
    std::vector<EnumInfo> enums;
    std::vector<ViewModelInfo> view_models;
};

// Helper function to convert a string to the specified case style
std::string toCaseHelper(const std::string& str, CaseStyle style)
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

std::string toCamelCase(const std::string& str)
{
    std::string result = toCaseHelper(str, CaseStyle::Camel);
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

std::string toPascalCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Pascal);
}

std::string toSnakeCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Snake);
}

std::string toKebabCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Kebab);
}

std::string sanitizeString(const std::string& input)
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

static std::unique_ptr<rive::File> open_file(const char name[])
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

static std::vector<std::string> get_animations_from_artboard(
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

std::vector<std::pair<std::string, std::vector<InputInfo>>>
get_state_machines_from_artboard(rive::ArtboardInstance* artboard)
{
    std::vector<std::pair<std::string, std::vector<InputInfo>>> state_machines;
    auto stateMachineCount = artboard->stateMachineCount();
    for (int i = 0; i < stateMachineCount; i++)
    {
        auto stateMachine = artboard->stateMachineAt(i);
        std::string state_machine_name = stateMachine->name();

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

        state_machines.emplace_back(state_machine_name, inputs);
    }
    return state_machines;
}

std::vector<std::string> find_riv_files(const std::string& path)
{
    std::vector<std::string> riv_files;

    if (std::filesystem::is_directory(path))
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.path().extension() == ".riv")
            {
                riv_files.push_back(entry.path().string());
            }
        }
    }
    else if (std::filesystem::path(path).extension() == ".riv")
    {
        riv_files.push_back(path);
    }

    return riv_files;
}

std::string makeUnique(const std::string& base,
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

std::vector<TextValueRunInfo> get_text_value_runs_from_artboard(
    rive::ArtboardInstance* artboard)
{
    std::vector<rive::TextValueRun*> text_value_runs;
    std::vector<TextValueRunInfo> text_value_runs_info;

    findAll<rive::TextValueRun>(text_value_runs, artboard);

    for (auto text_value_run : text_value_runs)
    {
        if (!text_value_run->name().empty())
        {
            text_value_runs_info.push_back(
                {text_value_run->name(), text_value_run->text()});
        }
    }
    return text_value_runs_info;
}

std::vector<NestedTextValueRunInfo>
get_nested_text_value_run_paths_from_artboard(
    rive::ArtboardInstance* artboard,
    const std::string& current_path = "")
{
    std::vector<NestedTextValueRunInfo> nested_text_value_runs_info;
    auto count = artboard->nestedArtboards().size();

    if (!current_path.empty())
    {
        auto text_runs = get_text_value_runs_from_artboard(artboard);
        for (const auto& text_run : text_runs)
        {
            nested_text_value_runs_info.push_back(
                {text_run.name, current_path});
        }
    }

    // Recursively process nested artboards
    for (int i = 0; i < count; i++)
    {
        auto nested = artboard->nestedArtboards()[i];
        auto nested_name = nested->name();
        if (!nested_name.empty())
        {
            // Only process nested artboards that have an exported name
            std::string new_path = current_path.empty()
                                       ? nested->name()
                                       : current_path + "/" + nested->name();

            auto nested_results = get_nested_text_value_run_paths_from_artboard(
                nested->artboardInstance(),
                new_path);
            nested_text_value_runs_info.insert(
                nested_text_value_runs_info.end(),
                nested_results.begin(),
                nested_results.end());
        }
    }

    return nested_text_value_runs_info;
}

std::vector<AssetInfo> get_assets_from_file(rive::File* file)
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

std::string dataTypeToString(rive::DataType type)
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

std::optional<RiveFileData> process_riv_file(const std::string& rive_file_path)
{
    // Check if the file is empty
    if (std::filesystem::is_empty(rive_file_path))
    {
        std::cerr << "Error: Rive file is empty: " << rive_file_path
                  << std::endl;
        return std::nullopt;
    }

    auto riveFile = open_file(rive_file_path.c_str());
    if (!riveFile)
    {
        std::cerr << "Error: Failed to parse Rive file: " << rive_file_path
                  << std::endl;
        return std::nullopt;
    }

    std::filesystem::path path(rive_file_path);
    std::string file_name_without_extension = path.stem().string();
    std::vector<AssetInfo> assets = get_assets_from_file(riveFile.get());
    RiveFileData file_data;
    file_data.riv_pascal_case = toPascalCase(file_name_without_extension);
    file_data.riv_camel_case = toCamelCase(file_name_without_extension);
    file_data.riv_snake_case = toSnakeCase(file_name_without_extension);
    file_data.riv_kebab_case = toKebabCase(file_name_without_extension);
    file_data.assets = assets;

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
            file_data.enums.push_back(enumInfo);
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
            file_data.view_models.push_back(viewModelInfo);
        }
    }

    std::unordered_set<std::string> usedArtboardNames;

    auto artboardCount = riveFile->artboardCount();
    for (int i = 0; i < artboardCount; i++)
    {
        auto artboard = riveFile->artboardAt(i);
        std::string artboard_name = artboard->name();

        std::string artboard_pascal_case = toPascalCase(artboard_name);
        std::string artboard_camel_case = toCamelCase(artboard_name);
        std::string artboard_snake_case = toSnakeCase(artboard_name);
        std::string artboard_kebab_case = toKebabCase(artboard_name);

        // Ensure unique artboard variable names
        artboard_camel_case =
            makeUnique(artboard_camel_case, usedArtboardNames);

        std::vector<std::string> animations =
            get_animations_from_artboard(artboard.get());
        std::vector<std::pair<std::string, std::vector<InputInfo>>>
            state_machines = get_state_machines_from_artboard(artboard.get());
        std::vector<TextValueRunInfo> text_value_runs =
            get_text_value_runs_from_artboard(artboard.get());
        std::vector<NestedTextValueRunInfo> nested_text_value_runs =
            get_nested_text_value_run_paths_from_artboard(artboard.get());

        file_data.artboards.push_back({artboard_name,
                                       artboard_pascal_case,
                                       artboard_camel_case,
                                       artboard_snake_case,
                                       artboard_kebab_case,
                                       animations,
                                       state_machines,
                                       text_value_runs,
                                       nested_text_value_runs});
    }

    return file_data;
}

std::optional<std::string> read_template_file(const std::string& path)
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

    std::string input_path;
    std::string output_file_path;
    std::string template_path;
    Language language = Language::Dart; // Default to Dart

    app.add_option("-i, --input",
                   input_path,
                   "Path to Rive file or directory containing Rive files")
        ->required()
        ->check(CLI::ExistingFile | CLI::ExistingDirectory);

    app.add_option("-o, --output", output_file_path, "Output file path")
        ->required();

    app.add_option("-t,--template", template_path, "Custom template file path");

    app.add_option("-l, --language",
                   language,
                   "Programming language for code generation")
        ->transform(CLI::CheckedTransformer(
            std::map<std::string, Language>{{"dart", Language::Dart},
                                            {"js", Language::JavaScript}},
            CLI::ignore_case));

    CLI11_PARSE(app, argc, argv);

    std::string template_str;
    if (!template_path.empty())
    {
        auto custom_template = read_template_file(template_path);
        if (custom_template)
        {
            template_str = *custom_template;
            std::cout << "Using custom template from: " << template_path
                      << std::endl;
        }
        else
        {
            // TODO: This is probably not needed. Or can have a safety to
            // fallback to the language specified
            std::cout << "Falling back to default template." << std::endl;
            template_str = default_templates::DEFAULT_DART_TEMPLATE;
        }
    }
    else
    {
        if (language == Language::Dart)
        {
            template_str = default_templates::DEFAULT_DART_TEMPLATE;
        }
        else if (language == Language::JavaScript)
        {
            std::cout << "JavaScript code generation is not yet supported."
                      << std::endl;
            return 1;
        }
    }

    std::vector<std::string> riv_files = find_riv_files(input_path);

    if (riv_files.empty())
    {
        std::cerr << "No .riv files found in the specified path." << std::endl;
        return 1;
    }

    std::vector<RiveFileData> rive_file_data_list;
    for (const auto& riv_file : riv_files)
    {
        auto result = process_riv_file(riv_file);
        if (result)
        {
            rive_file_data_list.push_back(*result);
        }
        // If result is nullopt, the error has already been printed
    }

    // Mustache template rendering
    kainjow::mustache::data template_data;
    std::vector<kainjow::mustache::data> riv_file_list;

    for (size_t file_index = 0; file_index < rive_file_data_list.size();
         file_index++)
    {
        const auto& file_data = rive_file_data_list[file_index];
        kainjow::mustache::data riv_file_data;
        riv_file_data["riv_pascal_case"] = file_data.riv_pascal_case;
        riv_file_data["riv_camel_case"] = file_data.riv_camel_case;
        riv_file_data["riv_snake_case"] = file_data.riv_snake_case;
        riv_file_data["riv_kebab_case"] = file_data.riv_kebab_case;
        riv_file_data["last"] = (file_index == rive_file_data_list.size() - 1);

        // Add enums to template data
        std::vector<kainjow::mustache::data> enums;
        for (size_t enum_index = 0; enum_index < file_data.enums.size();
             enum_index++)
        {
            const auto& enum_info = file_data.enums[enum_index];
            kainjow::mustache::data enum_data;
            enum_data["enum_name"] = enum_info.name;
            enum_data["enum_camel_case"] = toCamelCase(enum_info.name);
            enum_data["enum_pascal_case"] = toPascalCase(enum_info.name);
            enum_data["enum_snake_case"] = toSnakeCase(enum_info.name);
            enum_data["enum_kebab_case"] = toKebabCase(enum_info.name);
            enum_data["last"] = (enum_index == file_data.enums.size() - 1);

            std::vector<kainjow::mustache::data> enum_values;
            for (size_t value_index = 0; value_index < enum_info.values.size();
                 value_index++)
            {
                const auto& value = enum_info.values[value_index];
                kainjow::mustache::data value_data;
                value_data["enum_value_key"] = value.key;
                value_data["enum_value_camel_case"] = toCamelCase(value.key);
                value_data["enum_value_pascal_case"] = toPascalCase(value.key);
                value_data["enum_value_snake_case"] = toSnakeCase(value.key);
                value_data["enum_value_kebab_case"] = toKebabCase(value.key);
                value_data["last"] =
                    (value_index == enum_info.values.size() - 1);
                enum_values.push_back(value_data);
            }
            enum_data["enum_values"] = enum_values;
            enums.push_back(enum_data);
        }
        riv_file_data["enums"] = enums;

        // Add view models to template data
        std::vector<kainjow::mustache::data> view_models;
        for (size_t vm_index = 0; vm_index < file_data.view_models.size();
             vm_index++)
        {
            const auto& view_model = file_data.view_models[vm_index];
            kainjow::mustache::data view_model_data;
            view_model_data["view_model_name"] = view_model.name;
            view_model_data["view_model_camel_case"] =
                toCamelCase(view_model.name);
            view_model_data["view_model_pascal_case"] =
                toPascalCase(view_model.name);
            view_model_data["view_model_snake_case"] =
                toSnakeCase(view_model.name);
            view_model_data["view_model_kebab_case"] =
                toKebabCase(view_model.name);
            view_model_data["last"] =
                (vm_index == file_data.view_models.size() - 1);

            std::vector<kainjow::mustache::data> properties;
            for (size_t prop_index = 0;
                 prop_index < view_model.properties.size();
                 prop_index++)
            {
                const auto& property = view_model.properties[prop_index];
                kainjow::mustache::data property_data;
                property_data["property_name"] = property.name;
                property_data["property_camel_case"] =
                    toCamelCase(property.name);
                property_data["property_pascal_case"] =
                    toPascalCase(property.name);
                property_data["property_snake_case"] =
                    toSnakeCase(property.name);
                property_data["property_kebab_case"] =
                    toKebabCase(property.name);
                property_data["property_type"] = property.type;

                // Add property type information for the viewmodel template
                kainjow::mustache::data property_type_data;
                property_type_data.set("is_view_model",
                                       property.type == "viewModel");
                property_type_data.set("is_enum", property.type == "enum");
                property_type_data.set("is_string", property.type == "string");
                property_type_data.set("is_number", property.type == "number");
                property_type_data.set("is_integer",
                                       property.type == "integer");
                property_type_data.set("is_boolean",
                                       property.type == "boolean");
                property_type_data.set("is_color", property.type == "color");
                property_type_data.set("is_list", property.type == "list");
                property_type_data.set("is_trigger",
                                       property.type == "trigger");
                property_type_data.set("backing_name", property.backing_name);
                property_type_data.set("backing_camel_case",
                                       toCamelCase(property.backing_name));
                property_type_data.set("backing_pascal_case",
                                       toPascalCase(property.backing_name));
                property_type_data.set("backing_snake_case",
                                       toSnakeCase(property.backing_name));
                property_type_data.set("backing_kebab_case",
                                       toKebabCase(property.backing_name));
                property_data.set("property_type", property_type_data);

                property_data["last"] =
                    (prop_index == view_model.properties.size() - 1);
                properties.push_back(property_data);
            }
            view_model_data["properties"] = properties;
            view_models.push_back(view_model_data);
        }
        riv_file_data["view_models"] = view_models;

        std::vector<kainjow::mustache::data> assets;
        for (size_t asset_index = 0; asset_index < file_data.assets.size();
             asset_index++)
        {
            const auto& asset = file_data.assets[asset_index];
            kainjow::mustache::data asset_data;
            asset_data["asset_name"] = asset.name;
            asset_data["asset_camel_case"] = toCamelCase(asset.name);
            asset_data["asset_pascal_case"] = toPascalCase(asset.name);
            asset_data["asset_snake_case"] = toSnakeCase(asset.name);
            asset_data["asset_kebab_case"] = toKebabCase(asset.name);
            asset_data["asset_type"] = asset.type;
            asset_data["asset_id"] = asset.asset_id;
            asset_data["asset_cdn_uuid"] = asset.cdn_uuid;
            asset_data["asset_cdn_base_url"] = asset.cdn_base_url;
            asset_data["last"] = (asset_index == file_data.assets.size() - 1);
            assets.push_back(asset_data);
        }
        riv_file_data["assets"] = assets;

        std::vector<kainjow::mustache::data> artboard_list;
        for (size_t artboard_index = 0;
             artboard_index < file_data.artboards.size();
             artboard_index++)
        {
            const auto& artboard = file_data.artboards[artboard_index];
            kainjow::mustache::data artboard_data;
            artboard_data["artboard_name"] = artboard.artboard_name;
            artboard_data["artboard_pascal_case"] =
                artboard.artboard_pascal_case;
            artboard_data["artboard_camel_case"] = artboard.artboard_camel_case;
            artboard_data["artboard_snake_case"] = artboard.artboard_snake_case;
            artboard_data["artboard_kebab_case"] = artboard.artboard_kebab_case;
            artboard_data["last"] =
                (artboard_index == file_data.artboards.size() - 1);

            std::unordered_set<std::string> usedAnimationNames;
            std::vector<kainjow::mustache::data> animations;
            for (size_t anim_index = 0; anim_index < artboard.animations.size();
                 anim_index++)
            {
                const auto& animation = artboard.animations[anim_index];
                kainjow::mustache::data anim_data;
                auto unique_name = makeUnique(animation, usedAnimationNames);
                anim_data["animation_name"] = animation;
                anim_data["animation_camel_case"] = toCamelCase(unique_name);
                anim_data["animation_pascal_case"] = toPascalCase(unique_name);
                anim_data["animation_snake_case"] = toSnakeCase(unique_name);
                anim_data["animation_kebab_case"] = toKebabCase(unique_name);
                anim_data["last"] =
                    (anim_index == artboard.animations.size() - 1);
                animations.push_back(anim_data);
            }
            artboard_data["animations"] = animations;

            std::unordered_set<std::string> usedStateMachineNames;
            std::vector<kainjow::mustache::data> state_machines;
            for (size_t sm_index = 0; sm_index < artboard.state_machines.size();
                 sm_index++)
            {
                const auto& state_machine = artboard.state_machines[sm_index];
                kainjow::mustache::data state_machine_data;
                auto unique_name =
                    makeUnique(state_machine.first, usedStateMachineNames);
                state_machine_data["state_machine_name"] = state_machine.first;
                state_machine_data["state_machine_camel_case"] =
                    toCamelCase(unique_name);
                state_machine_data["state_machine_pascal_case"] =
                    toPascalCase(unique_name);
                state_machine_data["state_machine_snake_case"] =
                    toSnakeCase(unique_name);
                state_machine_data["state_machine_kebab_case"] =
                    toKebabCase(unique_name);
                state_machine_data["last"] =
                    (sm_index == artboard.state_machines.size() - 1);

                std::unordered_set<std::string> usedInputNames;
                std::vector<kainjow::mustache::data> inputs;
                for (size_t input_index = 0;
                     input_index < state_machine.second.size();
                     input_index++)
                {
                    const auto& input = state_machine.second[input_index];
                    kainjow::mustache::data input_data;
                    auto unique_name = makeUnique(input.name, usedInputNames);
                    input_data["input_name"] = input.name;
                    input_data["input_camel_case"] = toCamelCase(unique_name);
                    input_data["input_pascal_case"] = toPascalCase(unique_name);
                    input_data["input_snake_case"] = toSnakeCase(unique_name);
                    input_data["input_kebab_case"] = toKebabCase(unique_name);
                    input_data["input_type"] = input.type;
                    input_data["input_default_value"] = input.default_value;
                    input_data["last"] =
                        (input_index == state_machine.second.size() - 1);
                    inputs.push_back(input_data);
                }
                state_machine_data["inputs"] = inputs;

                state_machines.push_back(state_machine_data);
            }
            artboard_data["state_machines"] = state_machines;

            std::unordered_set<std::string> usedTextValueRunNames;
            std::vector<kainjow::mustache::data> text_value_runs;
            for (size_t tvr_index = 0;
                 tvr_index < artboard.text_value_runs.size();
                 tvr_index++)
            {
                const auto& tvr = artboard.text_value_runs[tvr_index];
                kainjow::mustache::data tvr_data;
                auto unique_name = makeUnique(tvr.name, usedTextValueRunNames);
                tvr_data["text_value_run_name"] = tvr.name;
                tvr_data["text_value_run_camel_case"] =
                    toCamelCase(unique_name);
                tvr_data["text_value_run_pascal_case"] =
                    toPascalCase(unique_name);
                tvr_data["text_value_run_snake_case"] =
                    toSnakeCase(unique_name);
                tvr_data["text_value_run_kebab_case"] =
                    toKebabCase(unique_name);
                tvr_data["text_value_run_default"] = tvr.default_value;
                tvr_data["text_value_run_default_sanitized"] =
                    sanitizeString(tvr.default_value);
                tvr_data["last"] =
                    (tvr_index == artboard.text_value_runs.size() - 1);
                text_value_runs.push_back(tvr_data);
            }
            artboard_data["text_value_runs"] = text_value_runs;

            std::vector<kainjow::mustache::data> nested_text_value_runs;
            for (size_t ntvr_index = 0;
                 ntvr_index < artboard.nested_text_value_runs.size();
                 ntvr_index++)
            {
                const auto& ntvr = artboard.nested_text_value_runs[ntvr_index];
                kainjow::mustache::data ntvr_data;
                ntvr_data["nested_text_value_run_name"] = ntvr.name;
                ntvr_data["nested_text_value_run_path"] = ntvr.path;
                ntvr_data["last"] =
                    (ntvr_index == artboard.nested_text_value_runs.size() - 1);
                nested_text_value_runs.push_back(ntvr_data);
            }

            artboard_data["nested_text_value_runs"] = nested_text_value_runs;

            artboard_list.push_back(artboard_data);
        }

        riv_file_data["artboards"] = artboard_list;
        // Only add the section if there are view models or enums
        if (!view_models.empty() || !enums.empty())
        {
            riv_file_list.push_back(riv_file_data);
        }
    }

    template_data["generated_file_name"] = generated_file_name;
    template_data["riv_files"] = riv_file_list;

    kainjow::mustache::mustache tmpl(template_str);
    std::string result = tmpl.render(template_data);

    std::cout << "Rive: output_file_path = " << output_file_path << std::endl;

    std::filesystem::path output_path(output_file_path);

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