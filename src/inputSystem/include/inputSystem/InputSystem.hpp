//
// Created by divmone on 8/2/2026.
//

#pragma once
#include <SDL3/SDL.h>
#include <array>
class InputSystem {
public:
    void onKeyDown(SDL_Scancode scancode);
    void onKeyUp(SDL_Scancode scancode);
    void endFrame();

    bool isHeld(SDL_Scancode scancode) const;
    bool isPressed(SDL_Scancode scancode) const;
    bool isReleased(SDL_Scancode scancode) const;
private:
    std::array<bool, SDL_SCANCODE_COUNT> currentKeys_{};
    std::array<bool, SDL_SCANCODE_COUNT> previousKeys_{};
};