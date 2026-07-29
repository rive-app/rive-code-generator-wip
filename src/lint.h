#pragma once

#include "types.h"
#include "utils/no_op_factory.hpp"

#include "rive/assets/file_asset.hpp"
#include "rive/file_asset_loader.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <unordered_map>
#include <vector>

struct EmbeddedImageInfo
{
    size_t byteSize;
    std::string detectedExtension; // sniffed from magic bytes; empty if unrecognized
    uint32_t width = 0;
    uint32_t height = 0;
};

// Observes asset resolution during File::import to capture size and actual format
// of embedded images. loadContents receives the asset name and in-band bytes:
//   inBandBytes.size() > 0  → embedded (bytes are in the .riv file)
//   inBandBytes.size() == 0 → referenced or CDN (app/runtime supplies them)
// Returns false so normal decode proceeds via the factory.
class LintingAssetLoader : public rive::FileAssetLoader
{
public:
    // asset.name() → embedded info; absent for referenced/CDN images.
    std::unordered_map<std::string, EmbeddedImageInfo> embeddedImages;

    bool loadContents(rive::FileAsset& asset,
                      rive::Span<const uint8_t> inBandBytes,
                      rive::Factory* factory) override
    {
        if (inBandBytes.size() > 0)
        {
            auto dimensions = detectDimensions(inBandBytes);
            embeddedImages.emplace(asset.name(),
                                   EmbeddedImageInfo{inBandBytes.size(),
                                                     detectFormat(inBandBytes),
                                                     dimensions.first,
                                                     dimensions.second});
        }
        return false;
    }

private:
    static uint32_t readLittleEndian24(const uint8_t* data)
    {
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16);
    }

    static uint16_t readBigEndian16(const uint8_t* data)
    {
        return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 8) |
                                     static_cast<uint16_t>(data[1]));
    }

    static uint32_t readLittleEndian32(const uint8_t* data)
    {
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[3]) << 24);
    }

    static std::string detectFormat(rive::Span<const uint8_t> data)
    {
        if (data.size() >= 12 &&
            data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
            data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
        {
            return "webp";
        }
        if (data.size() >= 4 &&
            data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        {
            return "png";
        }
        if (data.size() >= 3 &&
            data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
        {
            return "jpeg";
        }
        if (data.size() >= 4 &&
            data[0] == 'G' && data[1] == 'I' && data[2] == 'F' && data[3] == '8')
        {
            return "gif";
        }
        return "";
    }

    static std::pair<uint32_t, uint32_t> detectDimensions(rive::Span<const uint8_t> data)
    {
        if (data.size() >= 12 &&
            data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
            data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
        {
            size_t offset = 12;
            while (offset + 8 <= data.size())
            {
                const auto* chunk = data.data() + offset;
                uint32_t chunkSize = readLittleEndian32(chunk + 4);
                if (offset + 8 + chunkSize > data.size())
                    break;

                if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'X' &&
                    chunkSize >= 10)
                {
                    const auto* payload = chunk + 8;
                    return {readLittleEndian24(payload + 4) + 1,
                            readLittleEndian24(payload + 7) + 1};
                }

                if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == 'L' &&
                    chunkSize >= 5)
                {
                    const auto* payload = chunk + 8;
                    if (payload[0] == 0x2F)
                    {
                        uint32_t bits = readLittleEndian32(payload + 1);
                        return {(bits & 0x3FFF) + 1, ((bits >> 14) & 0x3FFF) + 1};
                    }
                }

                if (chunk[0] == 'V' && chunk[1] == 'P' && chunk[2] == '8' && chunk[3] == ' ' &&
                    chunkSize >= 10)
                {
                    const auto* payload = chunk + 8;
                    return {static_cast<uint32_t>(payload[6] | ((payload[7] & 0x3F) << 8)),
                            static_cast<uint32_t>(payload[8] | ((payload[9] & 0x3F) << 8))};
                }

                offset += 8 + chunkSize + (chunkSize & 1);
            }
        }

        if (data.size() >= 24 &&
            data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' && data[3] == 'G')
        {
            return {readBigEndian16(data.data() + 18) |
                        (static_cast<uint32_t>(readBigEndian16(data.data() + 16)) << 16),
                    readBigEndian16(data.data() + 22) |
                        (static_cast<uint32_t>(readBigEndian16(data.data() + 20)) << 16)};
        }

        if (data.size() >= 4 && data[0] == 0xFF && data[1] == 0xD8)
        {
            size_t offset = 2;
            while (offset + 9 < data.size())
            {
                if (data[offset] != 0xFF)
                {
                    offset++;
                    continue;
                }

                uint8_t marker = data[offset + 1];
                if (marker == 0xC0 || marker == 0xC1 || marker == 0xC2)
                {
                    return {readBigEndian16(data.data() + offset + 7),
                            readBigEndian16(data.data() + offset + 5)};
                }

                if (marker == 0xD8 || marker == 0xD9)
                {
                    offset += 2;
                    continue;
                }

                uint16_t segmentSize = readBigEndian16(data.data() + offset + 2);
                if (segmentSize < 2)
                    break;
                offset += 2 + segmentSize;
            }
        }

        return {0, 0};
    }
};

std::vector<LintViolation> runLint(const std::vector<RiveFileData>& files,
                                    const LintConfig& config);

int outputLintViolations(const std::vector<LintViolation>& violations,
                          const LintConfig& config);
