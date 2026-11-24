//
// Created by divmone on 8/2/2026.
//

#pragma once
#include <SDL3/SDL.h>

#include <array>

class InputSystem {
public:
    void OnKeyDown(SDL_Scancode scancode);
    void OnKeyUp(SDL_Scancode scancode);
    void EndFrame();

    bool IsHeld(SDL_Scancode scancode) const;
    bool IsPressed(SDL_Scancode scancode) const;
    bool IsReleased(SDL_Scancode scancode) const;
private:
    std::array<bool, SDL_SCANCODE_COUNT> currentKeys_{};
    std::array<bool, SDL_SCANCODE_COUNT> previousKeys_{};
};