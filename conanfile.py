import os
from conan import ConanFile
from conan.tools.cmake import cmake_layout
from conan.tools.files import copy


class MyProjectConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"

    default_options = {
        "imgui/*:with_sdl3_binding": True,
    }

    def requirements(self):
        self.requires("sdl/3.4.8")
        self.requires("glm/1.0.3")
        self.requires("fastgltf/0.9.0")
        self.requires("stb/cci.20240531")
        self.requires("imgui/1.92.8")
        self.requires("nlohmann_json/3.12.0")
        self.requires("spdlog/1.17.0")
        self.requires("rmlui/6.2")

    def generate(self):
        imgui_res = os.path.join(self.dependencies["imgui"].package_folder, "res", "bindings")
        copy(self, "*sdl3*", imgui_res, os.path.join(self.source_folder, "bindings"))
        copy(self, "*vulkan*", imgui_res, os.path.join(self.source_folder, "bindings"))

    def layout(self):
        cmake_layout(self)
