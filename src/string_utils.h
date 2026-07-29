#pragma once

#include "types.h"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

inline std::string toCaseHelper(const std::string& str, CaseStyle style)
{
    std::stringstream result;
    bool capitalizeNext = (style == CaseStyle::Pascal);
    bool firstChar = true;

    if (std::isdigit(str[0]))
    {
        result << 'n';
        capitalizeNext = true;
        firstChar = false;
    }

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
    }

    std::string finalResult = result.str();
    if (finalResult.empty() || !std::isalpha(finalResult[0]))
    {
        finalResult = "X" + finalResult;
    }

    return finalResult;
}

inline std::string toCamelCase(const std::string& str)
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

inline std::string toPascalCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Pascal);
}

inline std::string toSnakeCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Snake);
}

inline std::string toKebabCase(const std::string& str)
{
    return toCaseHelper(str, CaseStyle::Kebab);
}

inline std::string sanitizeString(const std::string& input)
{
    std::string output;
    for (char c : input)
    {
        switch (c)
        {
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            case '\"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            default:
                if (std::isprint(c))
                {
                    output += c;
                }
                else
                {
                    char hex[7];
                    std::snprintf(hex, sizeof(hex), "\\u%04x", static_cast<unsigned char>(c));
                    output += hex;
                }
        }
    }
    return output;
}

inline std::string makeUnique(const std::string& base, std::unordered_set<std::string>& usedNames)
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
