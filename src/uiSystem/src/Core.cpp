//
// Created by divmone on 10/28/2026.
//

#include <uiSystem/UICore.hpp>
#include <vulkan/vulkan_raii.hpp>

UI::UI() {}

UI::~UI() {}

Rml::CompiledGeometryHandle UI::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    // Rml::CompiledGeometryHandle handle = Rml::C;
}

void UI::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                        Rml::Vector2f translation,
                        Rml::TextureHandle texture) {}

void UI::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {}

Rml::TextureHandle UI::LoadTexture(Rml::Vector2i& texture_dimensions,
                                   const Rml::String& source) {}

Rml::TextureHandle UI::GenerateTexture(Rml::Span<const unsigned char> source,
                                       Rml::Vector2i source_dimensions) {}

void UI::ReleaseTexture(Rml::TextureHandle texture) {}

void UI::EnableScissorRegion(bool enable) {}

void UI::SetScissorRegion(Rml::Rectanglei region) {}