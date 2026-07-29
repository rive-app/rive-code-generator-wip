#include "template_renderer.h"

#include "default_template.h"
#include "string_utils.h"
#include "kainjow/mustache.hpp"
#include "inja.hpp"
#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

static std::optional<std::string> readTemplateFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Warning: Unable to open template file: " << path << std::endl;
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());
}

std::optional<std::string> loadTemplate(const std::string& templatePath,
                                         Language language)
{
    if (!templatePath.empty())
    {
        auto customTemplate = readTemplateFile(templatePath);
        if (customTemplate)
        {
            std::cout << "Using custom template from: " << templatePath << std::endl;
            return customTemplate;
        }
        std::cout << "Falling back to default template." << std::endl;
        return default_templates::DEFAULT_DART_TEMPLATE;
    }

    if (language == Language::Dart)
    {
        return default_templates::DEFAULT_DART_TEMPLATE;
    }

    if (language == Language::JavaScript)
    {
        std::cout << "JavaScript code generation is not yet supported." << std::endl;
        return std::nullopt;
    }

    return std::nullopt;
}

static nlohmann::json buildInjaData(const std::string& generatedFileName,
                                     const std::vector<RiveFileData>& riveFileDataList)
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

        riveFileData["has_view_model"] = !fileData.viewmodels.empty();
        riveFileData["has_defaults"] = fileData.hasDefaults;
        riveFileData["default_artboard_name"] = fileData.defaultArtboardName;
        riveFileData["default_artboard_camel_case"] = toCamelCase(fileData.defaultArtboardName);
        riveFileData["default_state_machine_name"] = fileData.defaultStateMachineName;
        riveFileData["default_view_model_name"] = fileData.defaultViewModelName;

        // Enums
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
                enumValues.push_back(valueData);
            }
            enumData["enum_values"] = enumValues;
            enums.push_back(enumData);
        }
        riveFileData["enums"] = enums;

        // View models
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
                propertyTypeData["is_artboard"] = (property.type == "artboard");
                propertyTypeData["backing_name"] = property.backingName;
                propertyTypeData["backing_camel_case"] = toCamelCase(property.backingName);
                propertyTypeData["backing_pascal_case"] = toPascalCase(property.backingName);
                propertyTypeData["backing_snake_case"] = toSnakeCase(property.backingName);
                propertyTypeData["backing_kebab_case"] = toKebabCase(property.backingName);

                if (!property.defaultValue.empty())
                {
                    propertyTypeData["default_value"] = property.defaultValue;
                    if (property.type == "enum")
                    {
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

        // Assets
        nlohmann::json assets = nlohmann::json::array();
        for (size_t assetIndex = 0; assetIndex < fileData.assets.size(); assetIndex++)
        {
            const auto& asset = fileData.assets[assetIndex];
            nlohmann::json assetData;
            assetData["asset_name"] = asset.name;
            // The name exactly as authored in the .riv, without codegen's "U<n>" duplicate
            // disambiguation. Runtimes resolve referenced assets by "<raw name>-<asset id>"
            // (the exported uniqueFilename), so templates must use this — not asset_name —
            // wherever the value feeds a runtime asset lookup.
            assetData["asset_raw_name"] = asset.rawName;
            assetData["asset_camel_case"] = toCamelCase(asset.name);
            assetData["asset_pascal_case"] = toPascalCase(asset.name);
            assetData["asset_snake_case"] = toSnakeCase(asset.name);
            assetData["asset_kebab_case"] = toKebabCase(asset.name);
            assetData["asset_type"] = asset.type;
            assetData["asset_id"] = asset.assetId;
            assetData["asset_cdn_uuid"] = asset.cdnUuid;
            assetData["asset_cdn_base_url"] = asset.cdnBaseUrl;
            assets.push_back(assetData);
        }
        riveFileData["assets"] = assets;

        // Artboards
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
            artboardData["is_default"] = (artboard.artboardName == fileData.defaultArtboardName);
            artboardData["has_view_model"] = !artboard.viewModelName.empty();
            artboardData["view_model_id"] = static_cast<int>(artboard.viewModelId);
            artboardData["view_model_name"] = artboard.viewModelName;
            artboardData["default_state_machine_name"] = artboard.defaultStateMachineName;

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
                animations.push_back(animData);
            }
            artboardData["animations"] = animations;

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

                std::unordered_set<std::string> usedInputNames;
                nlohmann::json inputs = nlohmann::json::array();
                for (size_t inputIndex = 0; inputIndex < stateMachine.second.size(); inputIndex++)
                {
                    const auto& input = stateMachine.second[inputIndex];
                    nlohmann::json inputData;
                    auto uniqueInputName = makeUnique(input.name, usedInputNames);
                    inputData["input_name"] = input.name;
                    inputData["input_camel_case"] = toCamelCase(uniqueInputName);
                    inputData["input_pascal_case"] = toPascalCase(uniqueInputName);
                    inputData["input_snake_case"] = toSnakeCase(uniqueInputName);
                    inputData["input_kebab_case"] = toKebabCase(uniqueInputName);
                    inputData["input_type"] = input.type;
                    inputData["input_default_value"] = input.defaultValue;
                    inputs.push_back(inputData);
                }
                stateMachineData["inputs"] = inputs;
                stateMachines.push_back(stateMachineData);
            }
            artboardData["state_machines"] = stateMachines;

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
                textValueRuns.push_back(tvrData);
            }
            artboardData["text_value_runs"] = textValueRuns;

            nlohmann::json nestedTextValueRuns = nlohmann::json::array();
            for (size_t ntvrIndex = 0; ntvrIndex < artboard.nestedTextValueRuns.size(); ntvrIndex++)
            {
                const auto& ntvr = artboard.nestedTextValueRuns[ntvrIndex];
                nlohmann::json ntvrData;
                ntvrData["nested_text_value_run_name"] = ntvr.name;
                ntvrData["nested_text_value_run_path"] = ntvr.path;
                nestedTextValueRuns.push_back(ntvrData);
            }
            artboardData["nested_text_value_runs"] = nestedTextValueRuns;

            artboardList.push_back(artboardData);
        }
        riveFileData["artboards"] = artboardList;

        riveFileData["artboard_count"] = fileData.artboards.size();
        riveFileData["has_multiple_artboards"] = fileData.artboards.size() > 1;

        size_t totalAnimations = 0;
        size_t totalStateMachines = 0;
        for (const auto& artboard : fileData.artboards)
        {
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
        bool hasTypeSafeSwitching = fileData.artboards.size() > 1 ||
                                     (!hasViewModel && totalStateMachines == 0 && totalAnimations > 1) ||
                                     totalStateMachines > 1;
        riveFileData["has_type_safe_switching"] = hasTypeSafeSwitching;

        riveFileList.push_back(riveFileData);
    }

    data["riv_files"] = riveFileList;
    return data;
}

static kainjow::mustache::data buildMustacheData(const std::string& generatedFileName,
                                                   const std::vector<RiveFileData>& riveFileDataList)
{
    kainjow::mustache::data templateData;
    std::vector<kainjow::mustache::data> riveFileList;

    for (size_t fileIndex = 0; fileIndex < riveFileDataList.size(); fileIndex++)
    {
        const auto& fileData = riveFileDataList[fileIndex];
        kainjow::mustache::data riveFileData;
        riveFileData["riv_pascal_case"] = fileData.rivPascalCase;
        riveFileData["riv_camel_case"] = fileData.rivCameCase;
        riveFileData["riv_snake_case"] = fileData.riveSnakeCase;
        riveFileData["riv_kebab_case"] = fileData.rivKebabCase;
        riveFileData["last"] = (fileIndex == riveFileDataList.size() - 1);

        std::vector<kainjow::mustache::data> enums;
        for (size_t enumIndex = 0; enumIndex < fileData.enums.size(); enumIndex++)
        {
            const auto& enumInfo = fileData.enums[enumIndex];
            kainjow::mustache::data enumData;
            enumData["enum_name"] = enumInfo.name;
            enumData["enum_camel_case"] = toCamelCase(enumInfo.name);
            enumData["enum_pascal_case"] = toPascalCase(enumInfo.name);
            enumData["enum_snake_case"] = toSnakeCase(enumInfo.name);
            enumData["enum_kebab_case"] = toKebabCase(enumInfo.name);

            std::vector<kainjow::mustache::data> enumValues;
            for (size_t valueIndex = 0; valueIndex < enumInfo.values.size(); valueIndex++)
            {
                const auto& value = enumInfo.values[valueIndex];
                kainjow::mustache::data valueData;
                valueData["enum_value_key"] = value.key;
                valueData["enum_value_camel_case"] = toCamelCase(value.key);
                valueData["enum_value_pascal_case"] = toPascalCase(value.key);
                valueData["enum_value_snake_case"] = toSnakeCase(value.key);
                valueData["enum_value_kebab_case"] = toKebabCase(value.key);
                valueData["last"] = (valueIndex == enumInfo.values.size() - 1);
                enumValues.push_back(valueData);
            }
            enumData["enum_values"] = enumValues;
            enums.push_back(enumData);
        }
        riveFileData["enums"] = enums;

        std::vector<kainjow::mustache::data> viewmodels;
        for (size_t vmIndex = 0; vmIndex < fileData.viewmodels.size(); vmIndex++)
        {
            const auto& viewModel = fileData.viewmodels[vmIndex];
            kainjow::mustache::data viewmodelData;
            viewmodelData["view_model_name"] = viewModel.name;
            viewmodelData["view_model_camel_case"] = toCamelCase(viewModel.name);
            viewmodelData["view_model_pascal_case"] = toPascalCase(viewModel.name);
            viewmodelData["view_model_snake_case"] = toSnakeCase(viewModel.name);
            viewmodelData["view_model_kebab_case"] = toKebabCase(viewModel.name);
            viewmodelData["last"] = (vmIndex == fileData.viewmodels.size() - 1);

            std::vector<kainjow::mustache::data> properties;
            for (size_t propIndex = 0; propIndex < viewModel.properties.size(); propIndex++)
            {
                const auto& property = viewModel.properties[propIndex];
                kainjow::mustache::data propertyData;
                propertyData["property_name"] = property.name;
                propertyData["property_camel_case"] = toCamelCase(property.name);
                propertyData["property_pascal_case"] = toPascalCase(property.name);
                propertyData["property_snake_case"] = toSnakeCase(property.name);
                propertyData["property_kebab_case"] = toKebabCase(property.name);
                propertyData["property_type"] = property.type;

                kainjow::mustache::data propertyTypeData;
                propertyTypeData.set("is_view_model", property.type == "viewModel");
                propertyTypeData.set("is_enum", property.type == "enum");
                propertyTypeData.set("is_string", property.type == "string");
                propertyTypeData.set("is_number", property.type == "number");
                propertyTypeData.set("is_integer", property.type == "integer");
                propertyTypeData.set("is_boolean", property.type == "boolean");
                propertyTypeData.set("is_color", property.type == "color");
                propertyTypeData.set("is_list", property.type == "list");
                propertyTypeData.set("is_trigger", property.type == "trigger");
                propertyTypeData.set("is_artboard", property.type == "artboard");
                propertyTypeData.set("backing_name", property.backingName);
                propertyTypeData.set("backing_camel_case", toCamelCase(property.backingName));
                propertyTypeData.set("backing_pascal_case", toPascalCase(property.backingName));
                propertyTypeData.set("backing_snake_case", toSnakeCase(property.backingName));
                propertyTypeData.set("backing_kebab_case", toKebabCase(property.backingName));
                propertyData.set("property_type", propertyTypeData);

                propertyData["last"] = (propIndex == viewModel.properties.size() - 1);
                properties.push_back(propertyData);
            }
            viewmodelData["properties"] = properties;
            viewmodels.push_back(viewmodelData);
        }
        riveFileData["view_models"] = viewmodels;

        std::vector<kainjow::mustache::data> assets;
        for (size_t assetIndex = 0; assetIndex < fileData.assets.size(); assetIndex++)
        {
            const auto& asset = fileData.assets[assetIndex];
            kainjow::mustache::data assetData;
            assetData["asset_name"] = asset.name;
            // See the inja renderer above: raw .riv name for runtime asset lookups.
            assetData["asset_raw_name"] = asset.rawName;
            assetData["asset_camel_case"] = toCamelCase(asset.name);
            assetData["asset_pascal_case"] = toPascalCase(asset.name);
            assetData["asset_snake_case"] = toSnakeCase(asset.name);
            assetData["asset_kebab_case"] = toKebabCase(asset.name);
            assetData["asset_type"] = asset.type;
            assetData["asset_id"] = asset.assetId;
            assetData["asset_cdn_uuid"] = asset.cdnUuid;
            assetData["asset_cdn_base_url"] = asset.cdnBaseUrl;
            assets.push_back(assetData);
        }
        riveFileData["assets"] = assets;

        std::vector<kainjow::mustache::data> artboardList;
        for (size_t artboardIndex = 0; artboardIndex < fileData.artboards.size(); artboardIndex++)
        {
            const auto& artboard = fileData.artboards[artboardIndex];
            kainjow::mustache::data artboardData;
            artboardData["artboard_name"] = artboard.artboardName;
            artboardData["artboard_pascal_case"] = artboard.artboardPascalCase;
            artboardData["artboard_camel_case"] = artboard.artboardCameCase;
            artboardData["artboard_snake_case"] = artboard.artboardSnakeCase;
            artboardData["artboard_kebab_case"] = artboard.artboardKebabCase;
            artboardData["last"] = (artboardIndex == fileData.artboards.size() - 1);

            std::unordered_set<std::string> usedAnimationNames;
            std::vector<kainjow::mustache::data> animations;
            for (size_t animIndex = 0; animIndex < artboard.animations.size(); animIndex++)
            {
                const auto& animation = artboard.animations[animIndex];
                kainjow::mustache::data animData;
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

            std::unordered_set<std::string> usedStateMachineNames;
            std::vector<kainjow::mustache::data> stateMachines;
            for (size_t smIndex = 0; smIndex < artboard.stateMachines.size(); smIndex++)
            {
                const auto& stateMachine = artboard.stateMachines[smIndex];
                kainjow::mustache::data stateMachineData;
                auto uniqueName = makeUnique(stateMachine.first, usedStateMachineNames);
                stateMachineData["state_machine_name"] = stateMachine.first;
                stateMachineData["state_machine_camel_case"] = toCamelCase(uniqueName);
                stateMachineData["state_machine_pascal_case"] = toPascalCase(uniqueName);
                stateMachineData["state_machine_snake_case"] = toSnakeCase(uniqueName);
                stateMachineData["state_machine_kebab_case"] = toKebabCase(uniqueName);
                stateMachineData["last"] = (smIndex == artboard.stateMachines.size() - 1);

                std::unordered_set<std::string> usedInputNames;
                std::vector<kainjow::mustache::data> inputs;
                for (size_t inputIndex = 0; inputIndex < stateMachine.second.size(); inputIndex++)
                {
                    const auto& input = stateMachine.second[inputIndex];
                    kainjow::mustache::data inputData;
                    auto uniqueInputName = makeUnique(input.name, usedInputNames);
                    inputData["input_name"] = input.name;
                    inputData["input_camel_case"] = toCamelCase(uniqueInputName);
                    inputData["input_pascal_case"] = toPascalCase(uniqueInputName);
                    inputData["input_snake_case"] = toSnakeCase(uniqueInputName);
                    inputData["input_kebab_case"] = toKebabCase(uniqueInputName);
                    inputData["input_type"] = input.type;
                    inputData["input_default_value"] = input.defaultValue;
                    inputData["last"] = (inputIndex == stateMachine.second.size() - 1);
                    inputs.push_back(inputData);
                }
                stateMachineData["inputs"] = inputs;
                stateMachines.push_back(stateMachineData);
            }
            artboardData["state_machines"] = stateMachines;

            std::unordered_set<std::string> usedTextValueRunNames;
            std::vector<kainjow::mustache::data> textValueRuns;
            for (size_t tvrIndex = 0; tvrIndex < artboard.textValueRuns.size(); tvrIndex++)
            {
                const auto& tvr = artboard.textValueRuns[tvrIndex];
                kainjow::mustache::data tvrData;
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

            std::vector<kainjow::mustache::data> nestedTextValueRuns;
            for (size_t ntvrIndex = 0; ntvrIndex < artboard.nestedTextValueRuns.size(); ntvrIndex++)
            {
                const auto& ntvr = artboard.nestedTextValueRuns[ntvrIndex];
                kainjow::mustache::data ntvrData;
                ntvrData["nested_text_value_run_name"] = ntvr.name;
                ntvrData["nested_text_value_run_path"] = ntvr.path;
                ntvrData["last"] = (ntvrIndex == artboard.nestedTextValueRuns.size() - 1);
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
    return templateData;
}

std::optional<std::string> renderTemplate(const std::string& templateStr,
                                           const std::string& generatedFileName,
                                           const std::vector<RiveFileData>& data,
                                           TemplateEngine engine)
{
    if (engine == TemplateEngine::Mustache)
    {
        kainjow::mustache::mustache tmpl(templateStr);
        auto result = tmpl.render(buildMustacheData(generatedFileName, data));
        std::cout << "Using Mustache template engine" << std::endl;
        return result;
    }

    if (engine == TemplateEngine::Inja)
    {
        try
        {
            inja::Environment env;
            env.set_trim_blocks(true);
            env.set_lstrip_blocks(true);
            env.set_html_autoescape(false);
            env.set_throw_at_missing_includes(false);
            auto tmpl = env.parse(templateStr);
            auto result = env.render(tmpl, buildInjaData(generatedFileName, data));
            std::cout << "Using Inja template engine" << std::endl;
            return result;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: Inja rendering failed: " << e.what() << std::endl;
            return std::nullopt;
        }
    }

    std::cerr << "Error: Unknown template engine" << std::endl;
    return std::nullopt;
}
