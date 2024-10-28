#pragma once

#include <stringapiset.h>
#include <vector>
#include <algorithm>
#include <string>

static std::wstring ConvertUtf8ToWideString(const std::string_view utf8String)
{
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8String.data(), -1, nullptr, 0);
    if (bufferSize > 0)
    {
        std::vector<wchar_t> buffer(bufferSize);
        MultiByteToWideChar(CP_UTF8, 0, utf8String.data(), -1, buffer.data(), bufferSize);
        return std::wstring(buffer.data());
    }
    return L"";
}

/**
 * @brief Converts a wide string to a UTF-8 string
 * @param wideString The wide string to convert to a UTF-8 string
 * @return std::string The UTF-8 string
 */
static std::string ConvertWideStringToUtf8(std::wstring wideString);
/**
 *  This converts a wide string level name to a lowercase UTF-8 string
 * @param wideString The wide string to convert to a UTF-8 string 
 * @return std::string The UTF-8 string
 */
static std::string GetLowercasedLevelName(std::wstring wideString);

static std::string ConvertWideStringToString(std::wstring_view wideString)
{
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideString.data(), -1, nullptr, 0, nullptr, nullptr);
    if (bufferSize > 0)
    {
        std::vector<char> buffer(bufferSize);
        WideCharToMultiByte(CP_UTF8, 0, wideString.data(), -1, buffer.data(), bufferSize, nullptr, nullptr);
        return std::string(buffer.data());
    }
    return "";
}


static std::string GetLowercasedLevelName(const wchar_t* wideString)
{
        std::string LevelName = ConvertWideStringToString(wideString);
        std::transform(LevelName.begin(), LevelName.end(), LevelName.begin(), tolower);
    
    return "";
}