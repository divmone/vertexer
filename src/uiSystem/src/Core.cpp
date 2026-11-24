//
// Created by divmone on 10/28/2026.
//

#include <RmlUi_Platform_SDL.h>
#include <SDL3/SDL.h>

#include <FileSystem.hpp>
#include <algorithm>
#include <array>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <logSystem/Logger.hpp>
#include <mutex>
#include <renderSystem/2d/Vulkan2dContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <string_view>
#include <typesSystem/Vertex3d.hpp>
#include <uiSystem/UICore.hpp>
#include <vector>
#include <windowSystem/SDLWindow.hpp>

UI::UI() = default;

UI::~UI() {
    Shutdown();
}

bool UI::Init(SDLWindow& window, Vulkan2dContext& vulkan2dContext) {
    window_ = &window;
    vulkan2dContext_ = &vulkan2dContext;

    // RmlUi binds a texture descriptor even for untextured geometry. A 1x1
    // white texture keeps those draw calls valid when no other UI texture
    // exists yet.
    constexpr std::array<unsigned char, 4> whitePixel{255, 255, 255, 255};
    ImageData fallbackImage;
    fallbackImage.data = const_cast<unsigned char*>(whitePixel.data());
    fallbackImage.width = 1;
    fallbackImage.height = 1;
    fallbackImage.channels = 4;
    fallbackTexture_ = vulkan2dContext_->AddUiTexture(fallbackImage);
    fallbackImage.data = nullptr;

    Rml::SetSystemInterface(this);
    Rml::SetRenderInterface(this);

    if (!Rml::Initialise()) {
        LOG_ERROR("Failed to initialize RmlUi");
        return false;
    }

    const auto fontPath =
        FS::GetFullPath(FS::FileType::eAsset, "fonts/LatoLatin-Regular.ttf");

    if (!Rml::LoadFontFace(fontPath)) {
        LOG_ERROR("Failed to load RmlUi font: {}", fontPath);
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window.GetHandle(), &width, &height);

    context_ = Rml::CreateContext("main", Rml::Vector2i(width, height));

    if (context_ == nullptr) {
        LOG_ERROR("Failed to create RmlUi context");
        Rml::Shutdown();
        return false;
    }

    if (!LoadLayout()) {
        return false;
    }

    if (Rml::ElementDocument* document = layout_.GetDocument()) {
        if (Rml::Element* button = document->GetElementById("open-image")) {
            button->AddEventListener(Rml::EventId::Click, &openImageListener_);
        } else {
            LOG_ERROR("RmlUi: #open-image button not found");
        }
    }

    initialized_ = true;
    return true;
}

void UI::Shutdown() {
    if (!initialized_ && context_ == nullptr)
        return;

    context_ = nullptr;
    layout_ = AppBaseUiLayout{};

    Rml::Shutdown();

    if (vulkan2dContext_ != nullptr && fallbackTexture_ != 0) {
        vulkan2dContext_->ReleaseUiTexture(fallbackTexture_);
        fallbackTexture_ = 0;
    }

    initialized_ = false;
}

bool UI::LoadLayout() {
    if (context_ == nullptr)
        return false;

    const auto layoutPath =
        FS::GetFullPath(FS::FileType::eAsset, "ui/main.rml");
    return layout_.FromFile(*context_, layoutPath);
}

namespace {
// временная шиза от опенкода
Rml::String EscapeRml(const std::string& text) {
    Rml::String result;
    result.reserve(text.size());

    for (const char ch : text) {
        switch (ch) {
            case '&':
                result += "&amp;";
                break;
            case '<':
                result += "&lt;";
                break;
            case '>':
                result += "&gt;";
                break;
            default:
                result += ch;
                break;
        }
    }

    return result;
}

}  // namespace

void UI::OpenImageListener::ProcessEvent(Rml::Event& event) {
    (void)event;
    if (ui != nullptr) {
        ui->OpenImageDialog();
    }
}

void UI::OpenImageDialog() {
    // SDL requires the filter list to stay alive until the callback fires.
    static constexpr SDL_DialogFileFilter FILTERS[] = {
        {.name="Images",    .pattern="png;jpg;jpeg;bmp;tga;gif"},
        {.name="All files", .pattern="*"},
    };
// мб это можно и без сдла, я потом обязательно подшарю
    SDL_ShowOpenFileDialog(
        &UI::OnImageFileChosen,
        this,
        window_ != nullptr ? window_->GetHandle() : nullptr,
        FILTERS,
        std::size(FILTERS),
        nullptr,
        false);
}

void UI::OnImageFileChosen(void*              userdata,
                           const char* const* filelist,
                           int                filter) {
    (void)filter;

    auto* ui = static_cast<UI*>(userdata);
    if (ui == nullptr) {
        return;
    }

    if (filelist == nullptr) {
        LOG_ERROR("Image file dialog failed: {}", SDL_GetError());
        return;
    }

    if (filelist[0] == nullptr) {
        return;  // The user cancelled the dialog.
    }

    std::lock_guard<std::mutex> lock(ui->chosenImageMutex_);
    ui->chosenImagePath_ = filelist[0];
}

void UI::ApplyChosenImage() {
    // явно есть решение поэлегантнее но пока пусть будет
    // хорошо видеть что картинки пашут без спрайтов чисто на rml
    std::string path;
    {
        std::lock_guard<std::mutex> lock(chosenImageMutex_);
        if (chosenImagePath_.empty()) {
            return;
        }
        path.swap(chosenImagePath_);
    }

    Rml::ElementDocument* document = layout_.GetDocument();
    if (document == nullptr) {
        return;
    }

    Rml::Element* image = document->GetElementById("image-preview");
    if (image == nullptr) {
        return;
    }

    image->SetAttribute("src", Rml::String(path));
    image->SetProperty("display", "block");

    if (Rml::Element* placeholder =
            document->GetElementById("image-placeholder")) {
        placeholder->SetProperty("display", "none");
    }

    if (Rml::Element* status = document->GetElementById("image-status")) {
        const std::string name =
            std::filesystem::path(path).filename().string();
        status->SetInnerRML(Rml::String("Loaded: ") + EscapeRml(name));
    }
}

bool UI::ProcessEvent(SDL_Event& event) const {
    if (context_ == nullptr || window_ == nullptr)
        return false;

    return RmlSDL::InputEventHandler(context_, window_->GetHandle(), event);
}

void UI::Update() {
    ApplyChosenImage();

    if (context_ != nullptr)
        context_->Update();
}

void UI::Render() const {
    if (context_ != nullptr)
        context_->Render();
}

void UI::SetDimensions(int width, int height) const {
    if (context_ != nullptr)
        context_->SetDimensions(Rml::Vector2i(width, height));
}

Rml::CompiledGeometryHandle UI::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    if (vulkan2dContext_ == nullptr)
        return 0;

    std::vector<Vertex2d> convertedVertices;
    convertedVertices.reserve(vertices.size());

    for (const auto& vertex : vertices) {
        convertedVertices.push_back(
            {.pos = {vertex.position.x, vertex.position.y},
             .color = {static_cast<float>(vertex.colour.red) / 255.0f,
                       static_cast<float>(vertex.colour.green) / 255.0f,
                       static_cast<float>(vertex.colour.blue) / 255.0f,
                       static_cast<float>(vertex.colour.alpha) / 255.0f},
             .texCoord = {vertex.tex_coord.x, vertex.tex_coord.y}});
    }

    std::vector<uint32_t> convertedIndices;
    convertedIndices.reserve(indices.size());

    for (const int index : indices)
        convertedIndices.push_back(static_cast<uint32_t>(index));

    return vulkan2dContext_->AddUiGeometry(convertedVertices, convertedIndices);
}

void UI::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f               translation,
                        Rml::TextureHandle          texture) {
    if (vulkan2dContext_ == nullptr || geometry == 0)
        return;

    const glm::mat4 model = glm::translate(
        glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));

    const bool useScissor = scissorEnabled_ && scissorRegion_.Width() > 0 &&
                            scissorRegion_.Height() > 0;

    vk::Rect2D scissor{};
    if (useScissor) {
        scissor.offset = vk::Offset2D{.x = std::max(scissorRegion_.Left(), 0),
                                      .y = std::max(scissorRegion_.Top(), 0)};
        scissor.extent = vk::Extent2D{
            .width = static_cast<uint32_t>(scissorRegion_.Width()),
            .height = static_cast<uint32_t>(scissorRegion_.Height())};
    }

    const auto textureHandle =
        texture != 0 ? static_cast<std::size_t>(texture) : fallbackTexture_;

    vulkan2dContext_->AddUiDrawCall(static_cast<uint32_t>(geometry),
                                    model,
                                    textureHandle,
                                    texture != 0,
                                    useScissor,
                                    scissor);
}

void UI::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
    if (vulkan2dContext_ != nullptr && geometry != 0) {
        vulkan2dContext_->ReleaseUiGeometry(static_cast<uint32_t>(geometry));
    }
}

Rml::TextureHandle UI::LoadTexture(Rml::Vector2i&     textureDimensions,
                                   const Rml::String& source) {
    if (vulkan2dContext_ == nullptr || source.empty()) {
        return 0;
    }

    std::string path(source);
    std::replace(path.begin(), path.end(), '\\', '/');

    std::string fullPath = path;
    if (!std::filesystem::exists(fullPath)) {
        fullPath = FS::GetFullPath(FS::FileType::eAsset, path);
    }
    if (!std::filesystem::exists(fullPath)) {
        fullPath = FS::GetFullPath(FS::FileType::eAsset, "ui/" + path);
    }
    if (!std::filesystem::exists(fullPath)) {
        LOG_ERROR("UI texture not found: {}", source);
        return 0;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    if (stbi_info(fullPath.c_str(), &width, &height, &channels) == 0) {
        LOG_ERROR("UI texture is not a readable image: {}", fullPath);
        return 0;
    }

    ImageData image = AssetLoader::LoadImageDataFromFile(fullPath);
    if (image.data == nullptr || image.width == 0 || image.height == 0) {
        LOG_ERROR("Failed to load UI texture: {}", fullPath);
        return 0;
    }

    const auto handle = vulkan2dContext_->AddUiTexture(image);

    textureDimensions = {static_cast<int>(image.width),
                         static_cast<int>(image.height)};

    return static_cast<Rml::TextureHandle>(handle);
}

Rml::TextureHandle UI::GenerateTexture(Rml::Span<const Rml::byte> source,
                                       Rml::Vector2i sourceDimensions) {
    if (vulkan2dContext_ == nullptr || source.empty() ||
        sourceDimensions.x <= 0 || sourceDimensions.y <= 0) {
        return 0;
    }

    ImageData image;
    image.data = const_cast<unsigned char*>(source.data());
    image.width = static_cast<uint32_t>(sourceDimensions.x);
    image.height = static_cast<uint32_t>(sourceDimensions.y);
    image.channels = 4;

    const auto handle = vulkan2dContext_->AddUiTexture(image);

    // AddUiTexture copies the pixels synchronously; RmlUi owns this memory.
    image.data = nullptr;

    return static_cast<Rml::TextureHandle>(handle);
}

void UI::ReleaseTexture(Rml::TextureHandle texture) {
    if (vulkan2dContext_ != nullptr && texture != 0) {
        vulkan2dContext_->ReleaseUiTexture(static_cast<std::size_t>(texture));
    }
}

void UI::EnableScissorRegion(bool enable) {
    scissorEnabled_ = enable;
}

void UI::SetScissorRegion(Rml::Rectanglei region) {
    scissorRegion_ = region;
}

void UI::JoinPath(Rml::String&       translatedPath,
                  const Rml::String& documentPath,
                  const Rml::String& path) {
    // Paths returned by the native file dialog are absolute and must not be
    // rebased onto the document directory.
    const bool isAbsolute =
        !path.empty() &&
        (path.front() == '/' || path.front() == '\\' ||
         (path.size() > 2 && path[1] == ':'));

    if (isAbsolute) {
        translatedPath = path;
        return;
    }

    Rml::SystemInterface::JoinPath(translatedPath, documentPath, path);
}

double UI::GetElapsedTime() {
    static Uint64 start = SDL_GetPerformanceCounter();
    static auto frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    return static_cast<double>(SDL_GetPerformanceCounter() - start) / frequency;
}

bool UI::LogMessage(Rml::Log::Type type, const Rml::String& message) {
    switch (type) {
        case Rml::Log::LT_ERROR:
        case Rml::Log::LT_ASSERT:
            LOG_ERROR("RmlUi: {}", message);
            break;
        case Rml::Log::LT_WARNING:
            LOG_DEBUG("RmlUi warning: {}", message);
            break;
        case Rml::Log::LT_INFO:
            LOG_INFO("RmlUi: {}", message);
            break;
        case Rml::Log::LT_DEBUG:
            LOG_DEBUG("RmlUi: {}", message);
            break;
        default:
            LOG_INFO("RmlUi: {}", message);
            break;
    }

    return true;
}
