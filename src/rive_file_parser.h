#pragma once

#include "types.h"
#include "lint.h"

#include "rive/file.hpp"

#include <optional>
#include <string>
#include <vector>

std::vector<std::string> findRiveFiles(const std::string& path);

rive::rcp<rive::File> openFile(const char name[], rive::Factory& factory, rive::FileAssetLoader* loader = nullptr);

std::optional<RiveFileData> processRiveFile(const std::string& riveFilePath,
                                             bool ignorePrivate = false);
