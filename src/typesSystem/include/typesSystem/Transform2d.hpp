struct Transform2d {
    glm::vec2 pos_{0.0f};
    glm::vec2 scale_{1.0f};
    float rotateAngle_{0.0f};
    glm::mat4 model_{1.0f};

    glm::mat4 GetModelMat() const {
        glm::mat4 model(1.0f);

        model = glm::translate(model, glm::vec3(pos_, 0.0f));
        model = glm::rotate(model, rotateAngle_, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(scale_, 1.0f));

        return model;
    }
};