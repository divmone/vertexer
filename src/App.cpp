//
// Created by divmone on 10/16/2026.
//

#include <App.hpp>
#include <chrono>
#include <resourceSystem/SceneLoader.hpp>
#include <typesSystem/EngineStats.hpp>

#include "FileSystem.hpp"
#include "resourceSystem/AssetLoader.hpp"

App::App()
    : isRunning_(true),
      logger_("APP", true),
      window_("APP", 1000, 1000, true),
      vulkanContext_(window_.GetNativeHandler(), window_.GetExtentProvider()) {
    window_.SetOnEvent([&](SDL_Event& e) {
        if (e.type == SDL_EVENT_MOUSE_MOTION) {
            // if (mouseMode) {
            //     camera_.rotate(-e.motion.xrel, -e.motion.yrel);
            // }
        }
        if (e.type == SDL_EVENT_KEY_DOWN) {
            inputSystem_.onKeyDown(e.key.scancode);
        }
        if (e.type == SDL_EVENT_KEY_UP) {
            inputSystem_.onKeyUp(e.key.scancode);
        }
        if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
            vulkanContext_.recreateContext();
            return;
        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED ||
            e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            vulkanContext_.recreateContext();
        }
    });
    Init();
}

void App::Run() {
    EngineStats stats;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (isRunning_) {
        isRunning_ = window_.ProcessEvent();

        auto now = std::chrono::high_resolution_clock::now();

        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        Update(deltaTime);

        DrawSprites();
        vulkanContext_.renderFrame(scene_, stats);

        auto end = std::chrono::high_resolution_clock::now();

        auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(end - now);
        stats.frameTime = elapsed.count() / 1000.0f;
        inputSystem_.endFrame();
    }
}

void App::Init() {
    vulkanContext_.setProjMatrix(
        glm::perspective(glm::radians(45.0f),
                         static_cast<float>(window_.GetWidth()) /
                             static_cast<float>(window_.GetHeight()),
                         0.1f, 1000.0f));

    vulkanContext_.SetProj2dMatrix(glm::orthoZO(
        0.0f, static_cast<float>(window_.GetWidth()),
        static_cast<float>(window_.GetHeight()), 0.0f, 0.0f, 10.0f));

    InitScene();
}

void App::InitScene() {
    auto scenePath = FS::GetFullPath(FS::FileType::eScene, "scene");
    auto meshData = SceneLoader::LoadSceneMeshData(scenePath);

    for (const auto& model : meshData.models_) {
        vulkanContext_.AddModel(model);
    }

    SpriteData spriteData;
    constexpr std::array<Vertex2d, 4> quadVertices = {{
        // Position        Color                     UV
        {{-1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // Top-left
        {{1.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},   // Top-right
        {{1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},  // Bottom-right
        {{-1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},  // Bottom-left
    }};

    constexpr std::array<uint32_t, 6> quadIndices = {0, 1, 2, 2, 3, 0};

    spriteData.vertices_.assign(quadVertices.begin(), quadVertices.end());
    spriteData.indices_.assign(quadIndices.begin(), quadIndices.end());
    spriteData.image_ = AssetLoader::loadImageDataFromFile(
        FS::GetFullPath(FS::FileType::eAsset, "test2.jpg"));

    Sprite sprite;
    sprite.handle_ = vulkanContext_.AddSprite(spriteData);
    sprite.transform_.pos_ = {500.0f, 500.0f};
    sprite.transform_.scale_ = {100.0f, 100.0f};

    sprites_.push_back(sprite);

    sprite.transform_.pos_ = {2000.0f, 500.0f};
    sprite.transform_.scale_ = {500.0f, 500.0f};
    sprites_.push_back(sprite);

    Sprite sprite2;
    spriteData.image_ = AssetLoader::loadImageDataFromFile(
        FS::GetFullPath(FS::FileType::eAsset, "test.jpg"));
    sprite2.handle_ = vulkanContext_.AddSprite(spriteData);
    sprite2.transform_.pos_ = {0.0f, 0.0f};
    sprite2.transform_.scale_ = {300.0f, 300.0f};
    sprites_.push_back(sprite);

    // TODO fix this bad code ↓ ↓ ↓ ↓ ↓ ↓
    vulkanContext_.finishLoading();

    // TODO fix this bad code ↓ ↓ ↓ ↓ ↓ ↓
    scene_ = SceneLoader::LoadSceneFromJSON(scenePath);
}

void App::Update(float deltaTime) {
    if (inputSystem_.isHeld(SDL_SCANCODE_UP) ||
        inputSystem_.isHeld(SDL_SCANCODE_W)) {
        camera_.move(CameraDirections::Backward, deltaTime);
    }
    if (inputSystem_.isHeld(SDL_SCANCODE_DOWN) ||
        inputSystem_.isHeld(SDL_SCANCODE_S)) {
        camera_.move(CameraDirections::Forward, deltaTime);
    }
    if (inputSystem_.isHeld(SDL_SCANCODE_LEFT) ||
        inputSystem_.isHeld(SDL_SCANCODE_A)) {
        camera_.move(CameraDirections::Left, deltaTime);
    }
    if (inputSystem_.isHeld(SDL_SCANCODE_RIGHT) ||
        inputSystem_.isHeld(SDL_SCANCODE_D)) {
        camera_.move(CameraDirections::Right, deltaTime);
    }
    if (inputSystem_.isHeld(SDL_SCANCODE_E)) {
        camera_.move(CameraDirections::Up, deltaTime);
    }
    if (inputSystem_.isHeld(SDL_SCANCODE_Q)) {
        camera_.move(CameraDirections::Down, deltaTime);
    }
    if (inputSystem_.isPressed(SDL_SCANCODE_1)) {
        // mouseMode = !mouseMode;
        // SDL_SetWindowRelativeMouseMode(window_.getHandle(), mouseMode);
    }
    if (inputSystem_.isHeld(SDL_SCANCODE_ESCAPE)) {
        SDL_Event e;
        e.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&e);
    }

    camera_.update(deltaTime);

    vulkanContext_.setViewMatrix(camera_.getLookAt());
}

void App::DrawSprites() {
    for (const auto& sprite : sprites_) {
        vulkanContext_.DrawSprite(sprite);
    }
}