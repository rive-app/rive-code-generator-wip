#pragma once

#include "types.h"

#include <optional>
#include <string>
#include <vector>

std::optional<std::string> loadTemplate(const std::string& templatePath,
                                         Language language);

std::optional<std::string> renderTemplate(const std::string& templateStr,
                                           const std::string& generatedFileName,
                                           const std::vector<RiveFileData>& data,
                                           TemplateEngine engine);
