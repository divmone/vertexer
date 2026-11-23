//
// Created by divmone on 10/28/2026.
//

#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>

#include <cstdint>
#include <string>
#include <unordered_map>

#include "uiSystem/AppBaseUiLayout.hpp"

union SDL_Event;
class SDLWindow;
class VulkanContext;

class UI : public Rml::RenderInterface, public Rml::SystemInterface {
public:
    UI();
    ~UI() override;

    bool Init(SDLWindow& window, VulkanContext& vulkanContext);
    void Shutdown();

    void ProcessEvent(const SDL_Event& event);
    void Update();
    void Render();

    void SetDimensions(int width, int height);

    // --- Rml::RenderInterface ---
    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f translation,
                        Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& textureDimensions,
                                   const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(
        Rml::Span<const Rml::byte> source,
        Rml::Vector2i sourceDimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    // --- Rml::SystemInterface ---
    double GetElapsedTime() override;
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;

private:
    struct NamedTexture {
        Rml::TextureHandle handle{0};
        uint32_t width{0};
        uint32_t height{0};
    };

    bool LoadLayout();
    bool RegisterOffscreenImage(const std::string& name,
                                const std::string& path,
                                uint32_t width,
                                uint32_t height);

    SDLWindow* window_{nullptr};
    VulkanContext* vulkanContext_{nullptr};
    Rml::Context* context_{nullptr};
    AppBaseUiLayout layout_;

    std::unordered_map<std::string, NamedTexture> namedTextures_;

    bool scissorEnabled_{false};
    Rml::Rectanglei scissorRegion_{};
    bool initialized_{false};
};
