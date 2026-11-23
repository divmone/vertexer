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

    // не уверен насчёт этого всего
    window_.SetOnEvent([&](SDL_Event& e) {
        ui_.ProcessEvent(e);

        if (e.type == SDL_EVENT_KEY_DOWN) {
            inputSystem_.OnKeyDown(e.key.scancode);
        }
        if (e.type == SDL_EVENT_KEY_UP) {
            inputSystem_.OnKeyUp(e.key.scancode);
        }
        if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
            vulkanContext_.RecreateContext();
            return;
        }
        if (e.type == SDL_EVENT_QUIT) {

        }
        if (e.type == SDL_EVENT_WINDOW_RESIZED ||
            e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            vulkanContext_.RecreateContext();
        }
    });

    Init();
}

void App::Run() {
    using namespace std::chrono;
    using namespace std;

    EngineStats stats;
    auto lastTime = high_resolution_clock::now();

    while (isRunning_) {
        isRunning_ = window_.ProcessEvent();

        auto now = high_resolution_clock::now();
        float deltaTime = chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        Update(deltaTime);

        ui_.Update();
        ui_.Render();
        vulkanContext_.DrawSprites(stats);

        auto end = high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<microseconds>(end - now);
        stats.frameTime = elapsed.count() / 1000.0f;

        inputSystem_.EndFrame();
    }
}

void App::Init() {
    auto [width, height] = window_.GetExtentProvider().getExtentSizes();

    // RmlUi uses a top-left origin. Vulkan's Y axis points down in NDC, so
    // bottom/top are intentionally passed in this order.
    vulkanContext_.SetProj2dMatrix(
        glm::orthoZO(
            0.0f,
            static_cast<float>(width),
            0.0f,
            static_cast<float>(height),
            0.0f,
            10.0f));

    ui_.Init(window_, vulkanContext_);
}

void App::Update(float deltaTime) const {
    (void)deltaTime;
}
