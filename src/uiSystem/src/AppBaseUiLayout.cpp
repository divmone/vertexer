#include <uiSystem/AppBaseUiLayout.hpp>

#include <logSystem/Logger.hpp>

bool AppBaseUiLayout::FromFile(Rml::Context& context,
                               const std::string& path) {
    document_ = context.LoadDocument(path);
    if (document_ == nullptr) {
        LOG_ERROR("Failed to load RmlUi layout: {}", path);
        return false;
    }

    document_->Show();
    return true;
}

Rml::ElementDocument* AppBaseUiLayout::GetDocument() const {
    return document_;
}
