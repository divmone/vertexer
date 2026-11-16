//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <vector>

struct ExtensionInfo {
    std::string name;
};

struct IExtensionNamesReader {
    virtual ~IExtensionNamesReader() = default;
    virtual const char* GetExtensionNames() const = 0;
};