//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <cstdint>
#include <functional>

struct ExtentProvider {
    std::function<std::pair<std::uint32_t, std::uint32_t>()> GetExtentSizes;
};

struct IExtentProvider {
    virtual ~IExtentProvider() = default;
    virtual ExtentProvider GetExtentProvider() = 0;
};
