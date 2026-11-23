//
// Created by divmone on 8/2/2026.
//

#include <inputSystem/InputSystem.hpp>

void InputSystem::OnKeyDown(SDL_Scancode scancode) {
    currentKeys_[scancode] = true;
}

void InputSystem::OnKeyUp(SDL_Scancode scancode) {
    currentKeys_[scancode] = false;
}

void InputSystem::EndFrame() {
    previousKeys_ = currentKeys_;
}

bool InputSystem::IsHeld(SDL_Scancode scancode) const {
    return currentKeys_[scancode];
}

bool InputSystem::IsPressed(SDL_Scancode scancode) const {
    return currentKeys_[scancode] && !previousKeys_[scancode];
}

bool InputSystem::IsReleased(SDL_Scancode scancode) const {
    return !currentKeys_[scancode] && previousKeys_[scancode];
}
