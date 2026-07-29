#include "lint.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

std::vector<LintViolation> runLint(const std::vector<RiveFileData>& files,
                                    const LintConfig& config)
{
    std::vector<LintViolation> violations;

    for (const auto& fileData : files)
    {
        for (const auto& asset : fileData.assets)
        {
            if (asset.type != "image")
                continue;

            // Only lint embedded assets — referenced and CDN-hosted assets
            // are loaded at runtime by the app and are not our concern.
            if (asset.embeddedByteSize == 0)
                continue;

            // Format check
            const auto& ext = asset.fileExtension;
            bool allowed = std::find(config.allowedExtensions.begin(),
                                     config.allowedExtensions.end(),
                                     ext) != config.allowedExtensions.end();
            if (!allowed)
            {
                violations.push_back({LintViolation::Type::DisallowedAssetFormat,
                                      fileData.rivOriginalFileName,
                                      asset.name,
                                      ext});
            }

            // Size check
            if (asset.embeddedByteSize > config.maxAssetBytes)
            {
                violations.push_back({LintViolation::Type::OversizedAsset,
                                      fileData.rivOriginalFileName,
                                      asset.name,
                                      std::to_string(asset.embeddedByteSize)});
            }
        }

        for (const auto& artboard : fileData.artboards)
        {
            if (artboard.stateMachines.empty())
            {
                violations.push_back({LintViolation::Type::NoStateMachine,
                                      fileData.rivOriginalFileName,
                                      artboard.artboardName,
                                      ""});
            }
        }
    }

    return violations;
}

int outputLintViolations(const std::vector<LintViolation>& violations,
                          const LintConfig& config)
{
    if (config.jsonOutput)
    {
        nlohmann::json output = nlohmann::json::array();
        for (const auto& v : violations)
        {
            nlohmann::json vj;
            vj["file"] = v.fileName;
            switch (v.type)
            {
                case LintViolation::Type::DisallowedAssetFormat:
                    vj["type"] = "disallowed_asset_format";
                    vj["asset"] = v.name;
                    vj["format"] = v.detail;
                    break;
                case LintViolation::Type::OversizedAsset:
                    vj["type"] = "oversized_asset";
                    vj["asset"] = v.name;
                    vj["size_bytes"] = std::stoul(v.detail);
                    vj["max_bytes"] = config.maxAssetBytes;
                    break;
                case LintViolation::Type::NoStateMachine:
                    vj["type"] = "no_state_machine";
                    vj["artboard"] = v.name;
                    break;
            }
            output.push_back(vj);
        }
        std::cout << output.dump(2) << std::endl;
    }
    else
    {
        for (const auto& v : violations)
        {
            switch (v.type)
            {
                case LintViolation::Type::DisallowedAssetFormat:
                    std::cerr << "error: [" << v.fileName << "] asset '" << v.name
                              << "' has disallowed format '" << v.detail << "' (allowed: ";
                    for (size_t i = 0; i < config.allowedExtensions.size(); i++)
                    {
                        if (i > 0) std::cerr << ", ";
                        std::cerr << config.allowedExtensions[i];
                    }
                    std::cerr << ")" << std::endl;
                    break;
                case LintViolation::Type::OversizedAsset:
                {
                    size_t sizeKb = std::stoul(v.detail) / 1024;
                    size_t maxKb = config.maxAssetBytes / 1024;
                    std::cerr << "error: [" << v.fileName << "] asset '" << v.name
                              << "' is " << sizeKb << " KB (max: " << maxKb << " KB)"
                              << std::endl;
                    break;
                }
                case LintViolation::Type::NoStateMachine:
                    std::cerr << "error: [" << v.fileName << "] artboard '" << v.name
                              << "' has no state machines" << std::endl;
                    break;
            }
        }
    }

    return violations.empty() ? 0 : 1;
}
