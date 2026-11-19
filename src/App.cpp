//
// Created by divmone on 10/16/2026.
//

#include <App.hpp>
#include <chrono>

#include <glm/gtc/matrix_transform.hpp>
#include <typesSystem/EngineStats.hpp>

App::App()
    : isRunning_(true),
      logger_("APP", true),
      window_("APP", 1200, 800, false),
      vulkanContext_(window_.GetNativeHandler(), window_.GetExtentProvider()) {
    window_.SetOnEvent([&](SDL_Event& e) {
        ui_.ProcessEvent(e);

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
        float deltaTime =
            std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        Update(deltaTime);

        ui_.Update();
        ui_.Render();
        vulkanContext_.DrawSprites(stats);

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(end - now);
        stats.frameTime = elapsed.count() / 1000.0f;

        inputSystem_.endFrame();
    }
}

void App::Init() {
    auto [width, height] = window_.GetExtentProvider().GetExtentSizes();

    // RmlUi uses a top-left origin. Vulkan's Y axis points down in NDC, so
    // bottom/top are intentionally passed in this order.
    vulkanContext_.SetProj2dMatrix(glm::orthoZO(
        0.0f, static_cast<float>(width), 0.0f,
        static_cast<float>(height), 0.0f, 10.0f));

    ui_.Init(window_, vulkanContext_);
}

void App::Update(float deltaTime) {
    (void)deltaTime;

    if (inputSystem_.isHeld(SDL_SCANCODE_ESCAPE)) {
        SDL_Event e{};
        e.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&e);
    }
}
