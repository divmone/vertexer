//
// Created by divmone on 10/20/2026.
//

#pragma once

#include <stb_image.h>

struct ImageData {
    ImageData() = default;

    ImageData(const ImageData&)            = delete;
    ImageData& operator=(const ImageData&) = delete;

    ImageData(ImageData&& oth) noexcept
        : data(oth.data),
          width(oth.width),
          height(oth.height),
          channels(oth.channels) {
        oth.data = nullptr;
    }

    ImageData& operator=(ImageData&& oth) noexcept {
        if (this != &oth) {
            stbi_image_free(data);
            data     = oth.data;
            width    = oth.width;
            height   = oth.height;
            channels = oth.channels;
            oth.data = nullptr;
        }
        return *this;
    }

    ~ImageData() { stbi_image_free(data); }

    unsigned char* data{nullptr};
    uint32_t       width{0};
    uint32_t       height{0};
    uint8_t        channels{0};
};