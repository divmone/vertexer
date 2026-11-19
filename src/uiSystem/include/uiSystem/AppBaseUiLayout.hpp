#pragma once

#include <RmlUi/Core.h>

#include <string>

class AppBaseUiLayout {
public:
    bool FromFile(Rml::Context& context, const std::string& path);

    Rml::ElementDocument* GetDocument() const;

private:
    Rml::ElementDocument* document_{nullptr};
};
