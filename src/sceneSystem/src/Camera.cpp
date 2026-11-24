//
// Created by divmone on 2/3/2026.
//

#include <glm/ext/matrix_transform.hpp>
#include <sceneSystem/Camera.hpp>

const glm::mat4& Camera::GetLookAt() const {
    return view_;
}

void Camera::SetYaw(float yaw) {
    yaw_ = yaw;
}

void Camera::SetPitch(float pitch) {
    pitch_ = pitch;
}

void Camera::UpdateMatrix() {
    view_ = glm::lookAt(position_, position_ + front_, up_);
}

void Camera::UpdateVectors() {
    glm::vec3 tempFront;
    tempFront.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    tempFront.y = std::sin(glm::radians(pitch_));
    tempFront.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    front_ = glm::normalize(tempFront);
    right_ = glm::normalize(glm::cross(front_, worldUp_));
    up_ = glm::normalize(glm::cross(right_, front_));
}

void Camera::SetPosition(const glm::vec3& pos) {
    position_ = pos;
}

void Camera::Move(CameraDirections direction, float deltaTime) {
    glm::vec3 wishDir{0.0f};

    switch (direction) {
        case CameraDirections::Forward:
            wishDir -= front_;
            break;
        case CameraDirections::Backward:
            wishDir += front_;
            break;
        case CameraDirections::Right:
            wishDir += right_;
            break;
        case CameraDirections::Left:
            wishDir -= right_;
            break;
        case CameraDirections::Up:
            wishDir += up_;
            break;
        case CameraDirections::Down:
            wishDir -= up_;
            break;
    }

    float len =
        wishDir.x * wishDir.x + wishDir.y * wishDir.y + wishDir.z * wishDir.z;
    if (len > 0.0001f) {
        wishDir = glm::normalize(wishDir);
    }

    velocity_ += wishDir * speed_ * deltaTime;
}

void Camera::Update(const float deltaTime) {
    const float damping = std::pow(0.1f, deltaTime);
    velocity_ *= damping;
    position_ += velocity_ * deltaTime;
    UpdateVectors();
    UpdateMatrix();
}

const glm::vec3& Camera::GetPosition() const {
    return position_;
}

std::pair<float, float> Camera::GetRotation() const {
    return {yaw_, pitch_};
}

void Camera::Rotate(int deltaX, int deltaY) {
    yaw_ -= static_cast<float>(deltaX) * sensitivity_;
    pitch_ += static_cast<float>(deltaY) * sensitivity_;

    if (pitch_ > 89.0f) {
        pitch_ = 89.0f;
    }
    if (pitch_ < -89.0f) {
        pitch_ = -89.0f;
    }
}
