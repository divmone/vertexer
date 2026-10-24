//
// Created by divmone on 8/2/2026.
//

#include <inputSystem/InputSystem.hpp>

void InputSystem::onKeyDown(SDL_Scancode scancode) {
    currentKeys_[scancode] = true;
}

void InputSystem::onKeyUp(SDL_Scancode scancode) {
    currentKeys_[scancode] = false;
}

void InputSystem::endFrame() {
    previousKeys_ = currentKeys_;
}

bool InputSystem::isHeld(SDL_Scancode scancode) const {
    return currentKeys_[scancode];
}

bool InputSystem::isPressed(SDL_Scancode scancode) const {
    return currentKeys_[scancode] && !previousKeys_[scancode];
}

bool InputSystem::isReleased(SDL_Scancode scancode) const {
    return !currentKeys_[scancode] && previousKeys_[scancode];
}
