#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    // Сonstructor: look-at target
    Camera(glm::vec3 position,
           glm::vec3 target,
           glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f));

    // Constructor: custom yaw/pitch
    Camera(glm::vec3 position = glm::vec3(2.0f, 2.0f, 2.0f),
           glm::vec3 worldUp       = glm::vec3(0.0f, 0.0f, 1.0f),
           float yaw          = -90.0f,
           float pitch        =   0.0f);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    void ProcessKeyboard(GLFWwindow* window, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset);
    void ProcessVerticalMovement(float yoffset);
    void ProcessHorizontalMovement(float yoffset);
    void ProcessMouseScroll(float yoffset);

    // Camera parameters
    glm::vec3 Position;
    glm::vec3 Front{glm::vec3(0.0f, 0.0f, 1.0f)};
    glm::vec3 Up{glm::vec3(0.0f, 0.0f, 1.0f)};
    glm::vec3 Right;
    glm::vec3 WorldUp;

    float MovementSpeed{5.0f};
    float MouseSensitivity{0.1f};
    float Zoom{45.0f};

    float Yaw;
    float Pitch;

private:
    void updateCameraVectors();
};