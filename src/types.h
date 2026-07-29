#pragma once

#include <cstddef>
#include <string>
#include <vector>

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
    std::string name;    // disambiguated for codegen identifiers (second "SF-Pro" -> "SF-ProU1")
    std::string rawName; // as authored in the .riv; runtimes resolve assets by "<rawName>-<assetId>"
    std::string type;
    std::string fileExtension;
    std::string assetId;
    std::string cdnUuid;
    std::string cdnBaseUrl;
    size_t embeddedByteSize = 0; // 0 for CDN-hosted assets or non-image types
    uint32_t width = 0;
    uint32_t height = 0;
    size_t decodedRGBAByteSize = 0; // width * height * 4 for embedded image assets
};

struct ReferencedAssetInfo
{
    std::string name;
    std::string assetId;
    size_t decodedRGBAByteSize = 0;
};

struct ArtboardStats
{
    size_t objectCount = 0;
    size_t imageCount = 0;
    size_t shapeCount = 0;
    size_t pathCount = 0;
    size_t pathVertexCount = 0;
    size_t meshCount = 0;
    size_t meshVertexCount = 0;
    size_t clippingShapeCount = 0;
    size_t fillCount = 0;
    size_t strokeCount = 0;
    size_t gradientCount = 0;
    size_t trimPathCount = 0;
    size_t featherCount = 0;
    size_t boneCount = 0;
    size_t constraintCount = 0;
    size_t nestedArtboardCount = 0;
    std::vector<ReferencedAssetInfo> referencedAssets;
    size_t referencedDecodedRGBAByteSize = 0;
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
    std::string defaultValue;
};

struct ViewModelInfo
{
    std::string name;
    std::vector<PropertyInfo> properties;
};

struct ByteStatsBucket
{
    std::string name;
    size_t objectCount = 0;
    size_t byteSize = 0;
};

struct ByteStatsType
{
    std::string name;
    std::string category;
    uint32_t typeKey = 0;
    size_t objectCount = 0;
    size_t byteSize = 0;
};

struct ByteStatsPrimitive
{
    std::string name;
    size_t propertyCount = 0;
    size_t byteSize = 0;
};

struct ByteStats
{
    bool parsed = false;
    std::string error;
    size_t fileByteSize = 0;
    size_t headerByteSize = 0;
    size_t objectCount = 0;
    size_t parsedObjectByteSize = 0;
    std::vector<ByteStatsBucket> buckets;
    std::vector<ByteStatsType> topTypes;
    std::vector<ByteStatsPrimitive> primitives;
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

    bool isDefault;
    uint32_t viewModelId;
    std::string viewModelName;
    bool hasViewModel;
    std::string defaultStateMachineName;
    bool hasDefaultStateMachine;
    ArtboardStats stats;
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
    ByteStats byteStats;

    std::string defaultArtboardName;
    std::string defaultStateMachineName;
    std::string defaultViewModelName;
    bool hasDefaults;
};

struct LintConfig
{
    std::vector<std::string> allowedExtensions = {"webp"};
    size_t maxAssetBytes = 200 * 1024; // 200 KB
    bool jsonOutput = false;
};

struct LintViolation
{
    enum class Type
    {
        DisallowedAssetFormat,
        OversizedAsset,
        NoStateMachine,
    };
    Type type;
    std::string fileName;
    std::string name;   // asset name for asset violations, artboard name for SM violations
    std::string detail; // file extension for format violations, byte count for size violations
};

struct StatsConfig
{
    bool jsonOutput = false;
};
