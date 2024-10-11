#include <filesystem>
#include <iostream>
#include <fstream>
#include <optional>
#include "CLIUtils/CLI11.hpp"
#include "kainjow/mustache.hpp"
#include "rive/file.hpp"
#include "rive/animation/linear_animation_instance.hpp"
#include "rive/animation/state_machine_instance.hpp"
#include "utils/no_op_factory.hpp"
#include <cctype>
#include <sstream>
#include <vector>
#include <string>
#include "default_template.h"

const std::string generated_file_name = "rive_generated";

// Enum for different case styles
enum class CaseStyle
{
    Camel,
    Pascal,
    // Add future case styles here, e.g., Snake, Kebab, etc.
};

// Helper function to convert a string to the specified case style
std::string toCaseHelper(const std::string &str, CaseStyle style)
{
    std::stringstream result;
    bool capitalizeNext = (style == CaseStyle::Pascal);

    // Check if the first character is a digit
    if (std::isdigit(str[0]))
    {
        result << 'n';         // Prepend 'n' for number
        capitalizeNext = true; // Capitalize the first digit
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
                result << (style == CaseStyle::Pascal ? c : (char)std::tolower(c));
            }
        }
        else if (c == ' ' || c == '_' || c == '-')
        {
            capitalizeNext = true;
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

std::string toCamelCase(const std::string &str)
{
    return toCaseHelper(str, CaseStyle::Camel);
}

std::string toPascalCase(const std::string &str)
{
    return toCaseHelper(str, CaseStyle::Pascal);
}

struct ArtboardData
{
    std::string artboard_name;
    std::string artboard_class_name;
    std::string artboard_variable;
    std::vector<std::string> animations;
    std::vector<std::string> state_machines;
};

struct RiveFileData
{
    std::string riv_class;
    std::string riv_variable;
    std::vector<ArtboardData> artboards;
};

static std::unique_ptr<rive::File> open_file(const char name[])
{
    FILE *f = fopen(name, "rb");
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

static std::vector<std::string> get_animations_from_artboard(rive::ArtboardInstance *artboard)
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

std::vector<std::string> get_state_machines_from_artboard(rive::ArtboardInstance *artboard)
{
    std::vector<std::string> state_machines;
    auto stateMachineCount = artboard->stateMachineCount();
    for (int i = 0; i < stateMachineCount; i++)
    {
        auto stateMachine = artboard->stateMachineAt(i);
        std::string state_machine_name = stateMachine->name();

        std::string state_machine_variable = state_machine_name;

        state_machines.push_back(state_machine_variable);
    }
    return state_machines;
}

std::vector<std::string> find_riv_files(const std::string &path)
{
    std::vector<std::string> riv_files;

    if (std::filesystem::is_directory(path))
    {
        for (const auto &entry : std::filesystem::directory_iterator(path))
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

std::string makeUnique(const std::string &base, std::unordered_set<std::string> &usedNames)
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

std::optional<RiveFileData> process_riv_file(const std::string &rive_file_path)
{
    // Check if the file is empty
    if (std::filesystem::is_empty(rive_file_path))
    {
        std::cerr << "Error: Rive file is empty: " << rive_file_path << std::endl;
        return std::nullopt;
    }

    auto riveFile = open_file(rive_file_path.c_str());
    if (!riveFile)
    {
        std::cerr << "Error: Failed to parse Rive file: " << rive_file_path << std::endl;
        return std::nullopt;
    }

    std::filesystem::path path(rive_file_path);
    std::string file_name_without_extension = path.stem().string();
    RiveFileData file_data;
    file_data.riv_class = toPascalCase(file_name_without_extension);
    file_data.riv_variable = toCamelCase(file_name_without_extension);

    std::unordered_set<std::string> usedArtboardNames;

    auto artboardCount = riveFile->artboardCount();
    for (int i = 0; i < artboardCount; i++)
    {
        auto artboard = riveFile->artboardAt(i);
        std::string artboard_name = artboard->name();

        std::string artboard_class_name = toPascalCase(artboard_name);
        std::string artboard_variable = toCamelCase(artboard_name);

        // Ensure unique artboard variable names
        artboard_variable = makeUnique(artboard_variable, usedArtboardNames);

        std::vector<std::string> animations = get_animations_from_artboard(artboard.get());
        std::vector<std::string> state_machines = get_state_machines_from_artboard(artboard.get());

        file_data.artboards.push_back({artboard_name, artboard_class_name, artboard_variable, animations, state_machines});
    }

    return file_data;
}

std::optional<std::string> read_template_file(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Warning: Unable to open template file: " << path << std::endl;
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

// Add this enum after the existing enums
enum class Language
{
    Dart,
    JavaScript
};

int main(int argc, char *argv[])
{
    CLI::App app{"Rive Code Generator"};

    std::string input_path;
    std::string output_file_path;
    std::string template_path;
    Language language = Language::Dart; // Default to Dart

    app.add_option("-i, --input", input_path, "Path to Rive file or directory containing Rive files")
        ->required()
        ->check(CLI::ExistingFile | CLI::ExistingDirectory);

    app.add_option("-o, --output", output_file_path, "Output file path")
        ->required();

    app.add_option("-t,--template", template_path, "Custom template file path");

    app.add_option("-l, --language", language, "Programming language for code generation")
        ->transform(CLI::CheckedTransformer(std::map<std::string, Language>{
                                                {"dart", Language::Dart},
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
            std::cout << "Using custom template from: " << template_path << std::endl;
        }
        else
        {
            // TODO: This is probably not needed. Or can have a safety to fallback to the language specified
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
            std::cout << "JavaScript code generation is not yet supported." << std::endl;
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
    for (const auto &riv_file : riv_files)
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

    for (const auto &file_data : rive_file_data_list)
    {
        kainjow::mustache::data riv_file_data;
        riv_file_data["riv_class"] = file_data.riv_class;
        riv_file_data["riv_variable"] = file_data.riv_variable;

        std::vector<kainjow::mustache::data> artboard_list;
        for (const auto &artboard : file_data.artboards)
        {
            kainjow::mustache::data artboard_data;
            artboard_data["artboard_name"] = artboard.artboard_name;
            artboard_data["artboard_class_name"] = artboard.artboard_class_name;
            artboard_data["artboard_variable"] = artboard.artboard_variable;

            std::unordered_set<std::string> usedAnimationNames;
            std::vector<kainjow::mustache::data> animations;
            for (const auto &animation : artboard.animations)
            {
                kainjow::mustache::data anim_data;
                anim_data["animation_name"] = animation;
                anim_data["animation_variable"] = makeUnique(toCamelCase(animation), usedAnimationNames);
                animations.push_back(anim_data);
            }
            artboard_data["animations"] = animations;

            std::unordered_set<std::string> usedStateMachineNames;
            std::vector<kainjow::mustache::data> state_machines;
            for (const auto &state_machine : artboard.state_machines)
            {
                kainjow::mustache::data state_machine_data;
                state_machine_data["state_machine_name"] = state_machine;
                state_machine_data["state_machine_variable"] = makeUnique(toCamelCase(state_machine), usedStateMachineNames);
                state_machines.push_back(state_machine_data);
            }
            artboard_data["state_machines"] = state_machines;

            artboard_list.push_back(artboard_data);
        }

        riv_file_data["artboards"] = artboard_list;
        riv_file_list.push_back(riv_file_data);
    }

    template_data["generated_file_name"] = generated_file_name;
    template_data["riv_files"] = riv_file_list;

    // Use template_str for rendering
    kainjow::mustache::mustache tmpl(template_str);
    std::string result = tmpl.render(template_data);

    // Debug print the output file path
    std::cout << "Rive: output_file_path = " << output_file_path << std::endl;

    std::filesystem::path output_path(output_file_path);

    // If only a filename is provided, use the current directory
    if (output_path.is_relative() && output_path.parent_path().empty())
    {
        output_path = std::filesystem::current_path() / output_path;
    }

    // Create directories if they don't exist (this won't do anything if it's just a filename)
    std::filesystem::create_directories(output_path.parent_path());

    // Open the output file
    std::ofstream output_file(output_path);
    if (!output_file.is_open())
    {
        std::cerr << "Error: Unable to open output file: " << output_path << std::endl;
        return 1;
    }
    output_file << result;
    output_file.close();

    std::cout << "Dart class generated successfully: " << output_path << std::endl;

    return 0;
}