#include "stats.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

namespace
{
double bytesToMiB(size_t bytes)
{
    return static_cast<double>(bytes) / 1024.0 / 1024.0;
}

std::string formatMiB(size_t bytes)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << bytesToMiB(bytes);
    return stream.str();
}

std::string formatPercent(size_t bytes, size_t totalBytes)
{
    std::ostringstream stream;
    double percent = totalBytes == 0 ? 0.0 : (static_cast<double>(bytes) * 100.0 /
                                              static_cast<double>(totalBytes));
    stream << std::fixed << std::setprecision(1) << percent;
    return stream.str();
}

std::string fit(std::string value, size_t width)
{
    if (value.size() > width)
    {
        if (width <= 1)
        {
            return value.substr(0, width);
        }
        return value.substr(0, width - 1) + ".";
    }
    return value;
}

void printCell(const std::string& value, size_t width)
{
    std::cout << std::left << std::setw(static_cast<int>(width)) << fit(value, width);
}

void printNumber(size_t value, size_t width)
{
    std::cout << std::right << std::setw(static_cast<int>(width)) << value;
}

void printMiB(size_t bytes, size_t width)
{
    std::cout << std::right << std::setw(static_cast<int>(width)) << formatMiB(bytes);
}

size_t totalEmbeddedImageBytes(const RiveFileData& file)
{
    size_t total = 0;
    for (const auto& asset : file.assets)
    {
        if (asset.type == "image")
        {
            total += asset.embeddedByteSize;
        }
    }
    return total;
}

size_t totalDecodedRGBABytes(const RiveFileData& file)
{
    size_t total = 0;
    for (const auto& asset : file.assets)
    {
        if (asset.type == "image")
        {
            total += asset.decodedRGBAByteSize;
        }
    }
    return total;
}

size_t embeddedImageCount(const RiveFileData& file)
{
    return static_cast<size_t>(std::count_if(file.assets.begin(),
                                             file.assets.end(),
                                             [](const auto& asset) {
                                                 return asset.type == "image" &&
                                                        asset.embeddedByteSize > 0;
                                             }));
}

nlohmann::json assetJson(const AssetInfo& asset)
{
    nlohmann::json value;
    value["name"] = asset.name;
    value["type"] = asset.type;
    value["file_extension"] = asset.fileExtension;
    value["asset_id"] = asset.assetId;
    value["embedded_bytes"] = asset.embeddedByteSize;
    value["width"] = asset.width;
    value["height"] = asset.height;
    value["decoded_rgba_bytes"] = asset.decodedRGBAByteSize;
    value["decoded_rgba_mb"] = bytesToMiB(asset.decodedRGBAByteSize);
    value["cdn_uuid"] = asset.cdnUuid;
    value["cdn_base_url"] = asset.cdnBaseUrl;
    return value;
}

nlohmann::json artboardJson(const ArtboardData& artboard)
{
    const auto& stats = artboard.stats;
    nlohmann::json value;
    value["name"] = artboard.artboardName;
    value["is_default"] = artboard.isDefault;
    value["animation_count"] = artboard.animations.size();
    value["state_machine_count"] = artboard.stateMachines.size();
    value["object_count"] = stats.objectCount;
    value["image_count"] = stats.imageCount;
    value["shape_count"] = stats.shapeCount;
    value["path_count"] = stats.pathCount;
    value["path_vertex_count"] = stats.pathVertexCount;
    value["mesh_count"] = stats.meshCount;
    value["mesh_vertex_count"] = stats.meshVertexCount;
    value["clipping_shape_count"] = stats.clippingShapeCount;
    value["fill_count"] = stats.fillCount;
    value["stroke_count"] = stats.strokeCount;
    value["gradient_count"] = stats.gradientCount;
    value["trim_path_count"] = stats.trimPathCount;
    value["feather_count"] = stats.featherCount;
    value["bone_count"] = stats.boneCount;
    value["constraint_count"] = stats.constraintCount;
    value["nested_artboard_count"] = stats.nestedArtboardCount;
    value["referenced_decoded_rgba_bytes"] = stats.referencedDecodedRGBAByteSize;
    value["referenced_decoded_rgba_mb"] = bytesToMiB(stats.referencedDecodedRGBAByteSize);

    nlohmann::json referencedAssets = nlohmann::json::array();
    for (const auto& asset : stats.referencedAssets)
    {
        referencedAssets.push_back({
            {"name", asset.name},
            {"asset_id", asset.assetId},
            {"decoded_rgba_bytes", asset.decodedRGBAByteSize},
            {"decoded_rgba_mb", bytesToMiB(asset.decodedRGBAByteSize)},
        });
    }
    value["referenced_assets"] = referencedAssets;
    return value;
}

nlohmann::json byteStatsJson(const ByteStats& stats)
{
    nlohmann::json value;
    value["parsed"] = stats.parsed;
    value["error"] = stats.error;
    value["file_bytes"] = stats.fileByteSize;
    value["header_bytes"] = stats.headerByteSize;
    value["object_count"] = stats.objectCount;
    value["parsed_object_bytes"] = stats.parsedObjectByteSize;

    nlohmann::json buckets = nlohmann::json::array();
    for (const auto& bucket : stats.buckets)
    {
        buckets.push_back({
            {"name", bucket.name},
            {"object_count", bucket.objectCount},
            {"bytes", bucket.byteSize},
            {"percent", stats.fileByteSize == 0
                            ? 0.0
                            : static_cast<double>(bucket.byteSize) * 100.0 /
                                  static_cast<double>(stats.fileByteSize)},
        });
    }
    value["buckets"] = buckets;

    nlohmann::json topTypes = nlohmann::json::array();
    for (const auto& type : stats.topTypes)
    {
        topTypes.push_back({
            {"name", type.name},
            {"category", type.category},
            {"type_key", type.typeKey},
            {"object_count", type.objectCount},
            {"bytes", type.byteSize},
            {"percent", stats.fileByteSize == 0
                            ? 0.0
                            : static_cast<double>(type.byteSize) * 100.0 /
                                  static_cast<double>(stats.fileByteSize)},
        });
    }
    value["top_types"] = topTypes;

    nlohmann::json primitives = nlohmann::json::array();
    for (const auto& primitive : stats.primitives)
    {
        primitives.push_back({
            {"name", primitive.name},
            {"property_count", primitive.propertyCount},
            {"bytes", primitive.byteSize},
            {"percent", stats.fileByteSize == 0
                            ? 0.0
                            : static_cast<double>(primitive.byteSize) * 100.0 /
                                  static_cast<double>(stats.fileByteSize)},
        });
    }
    value["primitive_value_bytes"] = primitives;
    return value;
}

void outputTextStats(const std::vector<RiveFileData>& files)
{
    for (const auto& file : files)
    {
        std::cout << file.rivOriginalFileName << std::endl;
        std::cout << "  Summary" << std::endl;
        std::cout << "    embedded images      " << embeddedImageCount(file) << std::endl;
        std::cout << "    embedded image MB    " << formatMiB(totalEmbeddedImageBytes(file))
                  << std::endl;
        std::cout << "    decoded RGBA MB      " << formatMiB(totalDecodedRGBABytes(file))
                  << std::endl;
        std::cout << "    artboards            " << file.artboards.size() << std::endl;

        const auto& byteStats = file.byteStats;
        if (byteStats.parsed)
        {
            std::cout << "  File byte distribution" << std::endl;
            std::cout << "    file bytes           " << byteStats.fileByteSize << std::endl;
            std::cout << "    parsed objects       " << byteStats.objectCount << std::endl;
            std::cout << "    ";
            printCell("Bucket", 38);
            printCell("Objects", 10);
            printCell("Bytes", 10);
            printCell("%", 8);
            std::cout << std::endl;
            for (const auto& bucket : byteStats.buckets)
            {
                std::cout << "    ";
                printCell(bucket.name, 38);
                printNumber(bucket.objectCount, 9);
                std::cout << " ";
                printNumber(bucket.byteSize, 9);
                std::cout << " ";
                printCell(formatPercent(bucket.byteSize, byteStats.fileByteSize), 7);
                std::cout << std::endl;
            }

            if (!byteStats.topTypes.empty())
            {
                std::cout << "  Largest serialized object types" << std::endl;
                std::cout << "    ";
                printCell("Type", 30);
                printCell("Category", 34);
                printCell("Objects", 10);
                printCell("Bytes", 10);
                printCell("%", 8);
                std::cout << std::endl;
                for (const auto& type : byteStats.topTypes)
                {
                    std::cout << "    ";
                    printCell(type.name, 30);
                    printCell(type.category, 34);
                    printNumber(type.objectCount, 9);
                    std::cout << " ";
                    printNumber(type.byteSize, 9);
                    std::cout << " ";
                    printCell(formatPercent(type.byteSize, byteStats.fileByteSize), 7);
                    std::cout << std::endl;
                }
            }
        }
        else if (!byteStats.error.empty())
        {
            std::cout << "  File byte distribution" << std::endl;
            std::cout << "    unavailable: " << byteStats.error << std::endl;
        }

        std::vector<AssetInfo> imageAssets;
        for (const auto& asset : file.assets)
        {
            if (asset.type == "image" && asset.embeddedByteSize > 0)
            {
                imageAssets.push_back(asset);
            }
        }
        std::sort(imageAssets.begin(), imageAssets.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.decodedRGBAByteSize > rhs.decodedRGBAByteSize;
        });

        if (!imageAssets.empty())
        {
            std::cout << "  Largest embedded images" << std::endl;
            std::cout << "    ";
            printCell("Name", 38);
            printCell("Size", 12);
            printCell("Decoded MB", 10);
            std::cout << std::endl;
            for (size_t i = 0; i < std::min<size_t>(imageAssets.size(), 5); i++)
            {
                const auto& asset = imageAssets[i];
                std::cout << "    ";
                printCell(asset.name, 38);
                printCell(std::to_string(asset.width) + "x" + std::to_string(asset.height), 12);
                printMiB(asset.decodedRGBAByteSize, 10);
                std::cout << std::endl;
            }
        }

        if (!file.artboards.empty())
        {
            std::cout << "  Artboard memory" << std::endl;
            std::cout << "    ";
            printCell("Artboard", 30);
            printCell("Assets", 8);
            printCell("Decoded", 10);
            printCell("Images", 8);
            printCell("Nested", 8);
            std::cout << std::endl;
            for (const auto& artboard : file.artboards)
            {
                const auto& stats = artboard.stats;
                std::cout << "    ";
                printCell(artboard.artboardName, 30);
                printNumber(stats.referencedAssets.size(), 7);
                std::cout << " ";
                printMiB(stats.referencedDecodedRGBAByteSize, 9);
                std::cout << " ";
                printNumber(stats.imageCount, 7);
                std::cout << " ";
                printNumber(stats.nestedArtboardCount, 7);
                std::cout << std::endl;
            }

            std::cout << "  Artboard complexity" << std::endl;
            std::cout << "    ";
            printCell("Artboard", 30);
            printCell("Objects", 9);
            printCell("Shapes", 8);
            printCell("Paths", 7);
            printCell("PathV", 7);
            printCell("Meshes", 8);
            printCell("MeshV", 7);
            printCell("Clips", 7);
            printCell("Grads", 7);
            printCell("Bones", 7);
            printCell("Constr", 8);
            std::cout << std::endl;
            for (const auto& artboard : file.artboards)
            {
                const auto& stats = artboard.stats;
                std::cout << "    ";
                printCell(artboard.artboardName, 30);
                printNumber(stats.objectCount, 8);
                std::cout << " ";
                printNumber(stats.shapeCount, 7);
                std::cout << " ";
                printNumber(stats.pathCount, 6);
                std::cout << " ";
                printNumber(stats.pathVertexCount, 6);
                std::cout << " ";
                printNumber(stats.meshCount, 7);
                std::cout << " ";
                printNumber(stats.meshVertexCount, 6);
                std::cout << " ";
                printNumber(stats.clippingShapeCount, 6);
                std::cout << " ";
                printNumber(stats.gradientCount, 6);
                std::cout << " ";
                printNumber(stats.boneCount, 6);
                std::cout << " ";
                printNumber(stats.constraintCount, 7);
                std::cout << std::endl;
            }
        }

        std::cout << std::endl;
    }
}

void outputJsonStats(const std::vector<RiveFileData>& files)
{
    nlohmann::json output = nlohmann::json::array();
    for (const auto& file : files)
    {
        nlohmann::json fileJson;
        fileJson["file"] = file.rivOriginalFileName;
        fileJson["embedded_image_count"] = embeddedImageCount(file);
        fileJson["embedded_image_bytes"] = totalEmbeddedImageBytes(file);
        fileJson["embedded_image_mb"] = bytesToMiB(totalEmbeddedImageBytes(file));
        fileJson["decoded_rgba_bytes"] = totalDecodedRGBABytes(file);
        fileJson["decoded_rgba_mb"] = bytesToMiB(totalDecodedRGBABytes(file));
        fileJson["artboard_count"] = file.artboards.size();
        fileJson["byte_stats"] = byteStatsJson(file.byteStats);

        nlohmann::json assets = nlohmann::json::array();
        for (const auto& asset : file.assets)
        {
            assets.push_back(assetJson(asset));
        }
        fileJson["assets"] = assets;

        nlohmann::json artboards = nlohmann::json::array();
        for (const auto& artboard : file.artboards)
        {
            artboards.push_back(artboardJson(artboard));
        }
        fileJson["artboards"] = artboards;

        output.push_back(fileJson);
    }

    std::cout << output.dump(2) << std::endl;
}
} // namespace

int outputStats(const std::vector<RiveFileData>& files, const StatsConfig& config)
{
    if (config.jsonOutput)
    {
        outputJsonStats(files);
    }
    else
    {
        outputTextStats(files);
    }
    return 0;
}
