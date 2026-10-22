#pragma once

#include "glm/glm.hpp"

enum class CameraDirections{
    Up, Down, Right, Left, Forward, Backward
};

class Camera {
public:
    Camera() = default;

    Camera(const glm::vec3 &pos, const glm::vec3 &worldUp = {0.0, 1.0, 0.0}, float yaw = 0.0f, float pitch = 0.0f):
            position(pos), worldUp(worldUp), yaw(yaw), pitch(pitch) {
        updateVectors();
        updateMatrix();
    }

    const glm::mat4 &getLookAt();
    void setYaw(float yaw);
    void setPitch(float pitch);
    void setPosition(const glm::vec3 &pos);

    void move(CameraDirections direction, float deltaTime);
    void rotate(int deltaX, int deltaY);

    void update(float deltaTime);

    const glm::vec3& getPosition() const;

    std::pair<float, float> getRotation() const;
private:
    void updateMatrix();
    void updateVectors();

    glm::vec3 position{0.0};
    glm::vec3 front{0.0, 0.0, 0.0};
    glm::vec3 up{0.0};
    glm::vec3 right{0.0};
    glm::vec3 worldUp{0.0, 1.0, 0.0};
    float yaw = 0.0f, pitch = 0.0f;
    glm::mat4 view{1.0};
    float speed = 10.0f;
    const float sensitivity = 0.1f;

    glm::vec3 velocity{0.0};
};
