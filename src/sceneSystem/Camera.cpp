//
// Created by divmone on 2/3/2026.
//

#include "Camera.hpp"

#include <glm/ext/matrix_transform.hpp>

const glm::mat4 &Camera::getLookAt() {
    return view;
}

void Camera::setYaw(float yaw) {
    Camera::yaw = yaw;
}


void Camera::setPitch(float pitch) {
    Camera::pitch = pitch;
}

void Camera::updateMatrix() {
    view = glm::lookAt(position, position + front, up);
}

void Camera::updateVectors() {
    glm::vec3 tempFront;
    tempFront.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    tempFront.y = std::sin(glm::radians(pitch));
    tempFront.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(tempFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::setPosition(const glm::vec3 &pos) {
    position = pos;
}

void Camera::move(CameraDirections direction, float deltaTime) {
    glm::vec3 wishDir{0.0f};

    switch (direction) {
        case CameraDirections::Forward:  wishDir -= front; break;
        case CameraDirections::Backward: wishDir += front; break;
        case CameraDirections::Right:    wishDir += right; break;
        case CameraDirections::Left:     wishDir -= right; break;
        case CameraDirections::Up:       wishDir += up;    break;
        case CameraDirections::Down:     wishDir -= up;    break;
    }

    float len = wishDir.x*wishDir.x + wishDir.y*wishDir.y + wishDir.z*wishDir.z;
    if (len > 0.0001f) {
        wishDir = glm::normalize(wishDir);
    }

    velocity += wishDir * speed * deltaTime;
}

void Camera::update(const float deltaTime) {
    const float damping = std::pow(0.1f, deltaTime);
    velocity *= damping;
    position += velocity * deltaTime;
    updateVectors();
    updateMatrix();
}

const glm::vec3 & Camera::getPosition() const {
    return position;
}

std::pair<float, float> Camera::getRotation() const {
    return {yaw, pitch};
}

void Camera::rotate(int deltaX, int deltaY) {
    yaw -= (float)deltaX * sensitivity;
    pitch += (float)deltaY * sensitivity;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}
