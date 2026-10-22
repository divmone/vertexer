#include <SDL3/SDL.h>

#include <App.hpp>

int main() {
    try {
        App app;
        app.Run();
    } catch (std::exception& e) {
        LOG_ERROR(e.what());
    }

    return 0;
}