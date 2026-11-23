#pragma once

#include <glm/glm.hpp>

enum class CameraDirections : std::uint8_t {
    Up, Down, Right, Left, Forward, Backward
};

class Camera {
public:
    Camera() = default;

    explicit Camera(const glm::vec3 &pos, const glm::vec3 &worldUp = {0.0, 1.0, 0.0}, float yaw = 0.0f, float pitch = 0.0f):
            position_(pos), worldUp_(worldUp), yaw_(yaw), pitch_(pitch) {
        UpdateVectors();
        UpdateMatrix();
    }

    const glm::mat4 &GetLookAt() const;
    void SetYaw(float yaw);
    void SetPitch(float pitch);
    void SetPosition(const glm::vec3 &pos);

    void Move(CameraDirections direction, float deltaTime);
    void Rotate(int deltaX, int deltaY);

    void Update(float deltaTime);

    const glm::vec3& GetPosition() const;

    std::pair<float, float> GetRotation() const;
private:
    void UpdateMatrix();
    void UpdateVectors();

    glm::vec3 position_{0.0};
    glm::vec3 front_{0.0, 0.0, 0.0};
    glm::vec3 up_{0.0};
    glm::vec3 right_{0.0};
    glm::vec3 worldUp_{0.0, 1.0, 0.0};
    float yaw_ = 0.0f, pitch_ = 0.0f;
    glm::mat4 view_{1.0};
    float speed_ = 10.0f;
    const float sensitivity_ = 0.1f;

    glm::vec3 velocity_{0.0};
};
