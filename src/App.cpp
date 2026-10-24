//
// Created by divmone on 10/16/2026.
//

#include <App.hpp>

#include <resourceSystem/SceneLoader.hpp>
#include <typesSystem/EngineStats.hpp>

App::App()
    : isRunning_(true),
      logger_("APP", true),
      window_("APP", 1000, 1000),
      vulkanContext_(window_.GetNativeHandler(), window_.GetExtentProvider()) {
    window_.SetOnEvent([&](SDL_Event& e) {

    });
    init();
}

void App::Run() {
    vulkanContext_.setProjMatrix(
        glm::perspective(glm::radians(45.0f),
                         static_cast<float>(window_.GetWidth()) /
                             static_cast<float>(window_.GetHeight()),
                         0.1f, 1000.0f));

    EngineStats stats;
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (isRunning_) {
        isRunning_ = window_.ProcessEvent();

        auto now = std::chrono::high_resolution_clock::now();

        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        update(deltaTime);

        vulkanContext_.drawScene(scene_, stats);

        auto end = std::chrono::system_clock::now();

        auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(end - now);
        stats.frameTime = elapsed.count() / 1000.0f;
        inputSystem_.endFrame();
    }
}

void App::init() {
    initScene();
}

void App::initScene() {
    scene_ = SceneLoader::loadSceneFromJSON(vulkanContext_,
                                            "../../../scene/scene.json");
}

void App::update(float deltaTime) {
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