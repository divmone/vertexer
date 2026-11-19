//
// Created by divmone on 10/28/2026.
//

#include <uiSystem/UICore.hpp>

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include <FileSystem.hpp>
#include <logSystem/Logger.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <resourceSystem/types/Sprite.hpp>
#include <resourceSystem/types/SpriteData.hpp>
#include <typesSystem/Vertex3d.hpp>
#include <windowSystem/SDLWindow.hpp>

namespace {
Rml::Input::KeyIdentifier ConvertKey(SDL_Keycode key) {
    if (key >= SDLK_A && key <= SDLK_Z) {
        return static_cast<Rml::Input::KeyIdentifier>(
            Rml::Input::KI_A + static_cast<int>(key - SDLK_A));
    }
    if (key >= SDLK_0 && key <= SDLK_9) {
        return static_cast<Rml::Input::KeyIdentifier>(
            Rml::Input::KI_0 + static_cast<int>(key - SDLK_0));
    }

    switch (key) {
        case SDLK_SPACE:
            return Rml::Input::KI_SPACE;
        case SDLK_ESCAPE:
            return Rml::Input::KI_ESCAPE;
        case SDLK_RETURN:
            return Rml::Input::KI_RETURN;
        case SDLK_KP_ENTER:
            return Rml::Input::KI_NUMPADENTER;
        case SDLK_BACKSPACE:
            return Rml::Input::KI_BACK;
        case SDLK_TAB:
            return Rml::Input::KI_TAB;
        case SDLK_DELETE:
            return Rml::Input::KI_DELETE;
        case SDLK_INSERT:
            return Rml::Input::KI_INSERT;
        case SDLK_HOME:
            return Rml::Input::KI_HOME;
        case SDLK_END:
            return Rml::Input::KI_END;
        case SDLK_PAGEUP:
            return Rml::Input::KI_PRIOR;
        case SDLK_PAGEDOWN:
            return Rml::Input::KI_NEXT;
        case SDLK_LEFT:
            return Rml::Input::KI_LEFT;
        case SDLK_RIGHT:
            return Rml::Input::KI_RIGHT;
        case SDLK_UP:
            return Rml::Input::KI_UP;
        case SDLK_DOWN:
            return Rml::Input::KI_DOWN;
        case SDLK_LSHIFT:
            return Rml::Input::KI_LSHIFT;
        case SDLK_RSHIFT:
            return Rml::Input::KI_RSHIFT;
        case SDLK_LCTRL:
            return Rml::Input::KI_LCONTROL;
        case SDLK_RCTRL:
            return Rml::Input::KI_RCONTROL;
        case SDLK_LALT:
            return Rml::Input::KI_LMENU;
        case SDLK_RALT:
            return Rml::Input::KI_RMENU;
        default:
            return Rml::Input::KI_UNKNOWN;
    }
}

int GetKeyModifierState() {
    const SDL_Keymod mod = SDL_GetModState();
    int state = 0;

    if ((mod & SDL_KMOD_CTRL) != 0) {
        state |= Rml::Input::KM_CTRL;
    }
    if ((mod & SDL_KMOD_SHIFT) != 0) {
        state |= Rml::Input::KM_SHIFT;
    }
    if ((mod & SDL_KMOD_ALT) != 0) {
        state |= Rml::Input::KM_ALT;
    }
    if ((mod & SDL_KMOD_GUI) != 0) {
        state |= Rml::Input::KM_META;
    }
    if ((mod & SDL_KMOD_CAPS) != 0) {
        state |= Rml::Input::KM_CAPSLOCK;
    }
    if ((mod & SDL_KMOD_NUM) != 0) {
        state |= Rml::Input::KM_NUMLOCK;
    }
    if ((mod & SDL_KMOD_SCROLL) != 0) {
        state |= Rml::Input::KM_SCROLLLOCK;
    }

    return state;
}

int ConvertMouseButton(uint8_t button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return 0;
        case SDL_BUTTON_MIDDLE:
            return 1;
        case SDL_BUTTON_RIGHT:
            return 2;
        default:
            return 0;
    }
}

constexpr std::string_view kOffscreenPrefix = "offscreen_";
}  // namespace

UI::UI() = default;

UI::~UI() {
    Shutdown();
}

bool UI::Init(SDLWindow& window, VulkanContext& vulkanContext) {
    window_ = &window;
    vulkanContext_ = &vulkanContext;

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

    context_ = Rml::CreateContext(
        "main", Rml::Vector2i(static_cast<int>(width),
                              static_cast<int>(height)));
    if (context_ == nullptr) {
        LOG_ERROR("Failed to create RmlUi context");
        Rml::Shutdown();
        return false;
    }

    // Render the existing assets into off-screen buffers first, then expose
    // those buffers as textures for the RmlUi document.
    if (!RegisterOffscreenImage(
            "test.jpg", FS::GetFullPath(FS::FileType::eAsset, "test.jpg"), 480,
            270)) {
        return false;
    }
    if (!RegisterOffscreenImage(
            "test2.jpg", FS::GetFullPath(FS::FileType::eAsset, "test2.jpg"),
            480, 270)) {
        return false;
    }

    if (!LoadLayout()) {
        return false;
    }

    initialized_ = true;
    return true;
}

void UI::Shutdown() {
    if (!initialized_ && context_ == nullptr) {
        return;
    }

    namedTextures_.clear();
    context_ = nullptr;
    layout_ = AppBaseUiLayout{};

    Rml::Shutdown();
    initialized_ = false;
}

bool UI::LoadLayout() {
    if (context_ == nullptr) {
        return false;
    }

    const auto layoutPath =
        FS::GetFullPath(FS::FileType::eAsset, "ui/main.rml");
    return layout_.FromFile(*context_, layoutPath);
}

bool UI::RegisterOffscreenImage(const std::string& name,
                                const std::string& path,
                                uint32_t width,
                                uint32_t height) {
    if (vulkanContext_ == nullptr) {
        return false;
    }

    constexpr std::array<Vertex2d, 4> quadVertices = {{
        {{-1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    }};
    constexpr std::array<uint32_t, 6> quadIndices = {0, 1, 2, 2, 3, 0};

    SpriteData spriteData;
    spriteData.vertices_.assign(quadVertices.begin(), quadVertices.end());
    spriteData.indices_.assign(quadIndices.begin(), quadIndices.end());
    spriteData.image_ = AssetLoader::loadImageDataFromFile(path);

    Sprite sprite;
    sprite.handle_ = vulkanContext_->AddSprite(spriteData);

    const auto target = vulkanContext_->CreateOffscreenTarget(width, height);
    vulkanContext_->RenderSpriteToOffscreen(sprite.handle_, target);

    const auto [targetWidth, targetHeight] =
        vulkanContext_->GetUiTextureSize(target);

    namedTextures_[name] = {
        .handle = target, .width = targetWidth, .height = targetHeight};

    return true;
}

void UI::ProcessEvent(const SDL_Event& event) {
    if (context_ == nullptr || window_ == nullptr) {
        return;
    }

    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION: {
            const float density =
                SDL_GetWindowPixelDensity(window_->GetHandle());
            context_->ProcessMouseMove(
                static_cast<int>(event.motion.x * density),
                static_cast<int>(event.motion.y * density),
                GetKeyModifierState());
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            context_->ProcessMouseButtonDown(
                ConvertMouseButton(event.button.button),
                GetKeyModifierState());
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            context_->ProcessMouseButtonUp(
                ConvertMouseButton(event.button.button),
                GetKeyModifierState());
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            context_->ProcessMouseWheel(-event.wheel.y,
                                        GetKeyModifierState());
            break;
        case SDL_EVENT_KEY_DOWN:
            context_->ProcessKeyDown(ConvertKey(event.key.key),
                                     GetKeyModifierState());
            if (event.key.key == SDLK_RETURN ||
                event.key.key == SDLK_KP_ENTER) {
                context_->ProcessTextInput('\n');
            }
            break;
        case SDL_EVENT_KEY_UP:
            context_->ProcessKeyUp(ConvertKey(event.key.key),
                                   GetKeyModifierState());
            break;
        case SDL_EVENT_TEXT_INPUT:
            context_->ProcessTextInput(Rml::String(event.text.text));
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
            int width = 0;
            int height = 0;
            SDL_GetWindowSizeInPixels(window_->GetHandle(), &width, &height);
            SetDimensions(width, height);
            break;
        }
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
            context_->SetDensityIndependentPixelRatio(
                SDL_GetWindowDisplayScale(window_->GetHandle()));
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            context_->ProcessMouseLeave();
            break;
        default:
            break;
    }
}

void UI::Update() {
    if (context_ != nullptr) {
        context_->Update();
    }
}

void UI::Render() {
    if (context_ != nullptr) {
        context_->Render();
    }
}

void UI::SetDimensions(int width, int height) {
    if (context_ != nullptr) {
        context_->SetDimensions(Rml::Vector2i(width, height));
    }
}

Rml::CompiledGeometryHandle UI::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices,
    Rml::Span<const int> indices) {
    if (vulkanContext_ == nullptr) {
        return 0;
    }

    std::vector<Vertex2d> convertedVertices;
    convertedVertices.reserve(vertices.size());

    for (const auto& vertex : vertices) {
        convertedVertices.push_back(
            {{vertex.position.x, vertex.position.y},
             {static_cast<float>(vertex.colour.red) / 255.0f,
              static_cast<float>(vertex.colour.green) / 255.0f,
              static_cast<float>(vertex.colour.blue) / 255.0f,
              static_cast<float>(vertex.colour.alpha) / 255.0f},
             {vertex.tex_coord.x, vertex.tex_coord.y}});
    }

    std::vector<uint32_t> convertedIndices;
    convertedIndices.reserve(indices.size());
    for (const int index : indices) {
        convertedIndices.push_back(static_cast<uint32_t>(index));
    }

    return static_cast<Rml::CompiledGeometryHandle>(
        vulkanContext_->AddUiGeometry(convertedVertices, convertedIndices));
}

void UI::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f translation,
                        Rml::TextureHandle texture) {
    if (vulkanContext_ == nullptr || geometry == 0) {
        return;
    }

    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f),
                       glm::vec3(translation.x, translation.y, 0.0f));

    const bool useScissor =
        scissorEnabled_ && scissorRegion_.Width() > 0 &&
        scissorRegion_.Height() > 0;

    vk::Rect2D scissor{};
    if (useScissor) {
        scissor.offset =
            vk::Offset2D{.x = std::max(scissorRegion_.Left(), 0),
                         .y = std::max(scissorRegion_.Top(), 0)};
        scissor.extent = vk::Extent2D{
            .width = static_cast<uint32_t>(scissorRegion_.Width()),
            .height = static_cast<uint32_t>(scissorRegion_.Height())};
    }

    vulkanContext_->AddUiDrawCall(
        static_cast<uint32_t>(geometry), model,
        static_cast<uintptr_t>(texture), texture != 0, useScissor, scissor);
}

void UI::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
    if (vulkanContext_ != nullptr && geometry != 0) {
        vulkanContext_->ReleaseUiGeometry(
            static_cast<uint32_t>(geometry));
    }
}

Rml::TextureHandle UI::LoadTexture(Rml::Vector2i& texture_dimensions,
                                   const Rml::String& source) {
    if (vulkanContext_ == nullptr) {
        return 0;
    }

    std::string path(source);
    std::replace(path.begin(), path.end(), '\\', '/');

    const auto filename = std::filesystem::path(path).filename().string();
    if (filename.starts_with(kOffscreenPrefix)) {
        const auto name = filename.substr(kOffscreenPrefix.size());
        const auto it = namedTextures_.find(name);
        if (it != namedTextures_.end()) {
            texture_dimensions = {
                static_cast<int>(it->second.width),
                static_cast<int>(it->second.height)};
            return it->second.handle;
        }
    }

    std::string fullPath = path;
    if (!std::filesystem::exists(fullPath)) {
        fullPath = FS::GetFullPath(FS::FileType::eAsset, path);
    }

    ImageData image = AssetLoader::loadImageDataFromFile(fullPath);
    const auto handle = vulkanContext_->AddUiTexture(image);

    texture_dimensions = {static_cast<int>(image.width),
                          static_cast<int>(image.height)};

    return static_cast<Rml::TextureHandle>(handle);
}

Rml::TextureHandle UI::GenerateTexture(
    Rml::Span<const Rml::byte> source,
    Rml::Vector2i source_dimensions) {
    if (vulkanContext_ == nullptr || source.empty()) {
        return 0;
    }

    ImageData image;
    image.data = const_cast<unsigned char*>(source.data());
    image.width = static_cast<uint32_t>(source_dimensions.x);
    image.height = static_cast<uint32_t>(source_dimensions.y);
    image.channels = 4;

    const auto handle = vulkanContext_->AddUiTexture(image);

    // ImageData does not own this memory.
    image.data = nullptr;

    return static_cast<Rml::TextureHandle>(handle);
}

void UI::ReleaseTexture(Rml::TextureHandle texture) {
    if (vulkanContext_ != nullptr && texture != 0) {
        vulkanContext_->ReleaseUiTexture(static_cast<uintptr_t>(texture));
    }
}

void UI::EnableScissorRegion(bool enable) {
    scissorEnabled_ = enable;
}

void UI::SetScissorRegion(Rml::Rectanglei region) {
    scissorRegion_ = region;
}

double UI::GetElapsedTime() {
    static const Uint64 start = SDL_GetPerformanceCounter();
    static const double frequency =
        static_cast<double>(SDL_GetPerformanceFrequency());

    return static_cast<double>(SDL_GetPerformanceCounter() - start) /
           frequency;
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
