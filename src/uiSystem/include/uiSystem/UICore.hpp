//
// Created by divmone on 10/28/2026.
//

#pragma once

#include <RmlUi/Core.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/SystemInterface.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include "uiSystem/AppBaseUiLayout.hpp"

union SDL_Event;
class SDLWindow;
class Vulkan2dContext;

class UI : public Rml::RenderInterface, public Rml::SystemInterface {
public:
    UI();
    ~UI() override;

    bool Init(SDLWindow& window, Vulkan2dContext& vulkan2dContext);
    void Shutdown();

    bool ProcessEvent(SDL_Event& event) const;
    void Update() const;
    void Render() const;

    void SetDimensions(int width, int height) const;

    // --- Rml::RenderInterface ---
    Rml::CompiledGeometryHandle CompileGeometry(
        Rml::Span<const Rml::Vertex> vertices,
        Rml::Span<const int>         indices) override;

    void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f               translation,
                        Rml::TextureHandle          texture) override;

    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i&     textureDimensions,
                                   const Rml::String& source) override;

    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
                                       Rml::Vector2i sourceDimensions) override;

    void ReleaseTexture(Rml::TextureHandle texture) override;
    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    // --- Rml::SystemInterface ---
    double GetElapsedTime() override;
    bool   LogMessage(Rml::Log::Type type, const Rml::String& message) override;

private:
    bool LoadLayout();

    SDLWindow*        window_{nullptr};
    Vulkan2dContext*  vulkan2dContext_{nullptr};
    Rml::Context*     context_{nullptr};
    AppBaseUiLayout   layout_;
    std::size_t       fallbackTexture_{0};

    bool            scissorEnabled_{false};
    Rml::Rectanglei scissorRegion_{};
    bool            initialized_{false};
};
