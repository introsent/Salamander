#include "camera.h"

#include <algorithm>

// Сonstructor: look-at target
Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 worldUp, float roll)
    : Position(position), WorldUp(worldUp), Roll(roll)
 {
    // Compute initial Front vector
    Front = glm::normalize(target - position);
    // Derive Euler angles from Front
    Pitch = glm::degrees(asin(Front.z));
    Yaw   = -glm::degrees(atan2(Front.y, Front.x));
    updateCameraVectors();
}

// Constructor: custom yaw/pitch
Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch, float roll)
    : Position(position), WorldUp(worldUp), Yaw(yaw), Pitch(pitch), Roll(roll) {
    updateCameraVectors();
}
glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    glm::mat4 proj = glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 100.0f);
    proj[1][1] *= -1.0f;
    return proj;
}

void Camera::ProcessKeyboard(GLFWwindow* window, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        velocity *= 3.0f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        Position += Front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        Position -= Front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        Position -= Right * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        Position += Right * velocity;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        Position += Up * velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        Position -= Up * velocity;

}


void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    Pitch = std::clamp(Pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void Camera::ProcessVerticalMovement(float yoffset) {
    float velocity = yoffset * MouseSensitivity * 0.25f;
    Position += WorldUp * velocity;
}

void Camera::ProcessHorizontalMovement(float yoffset) {
    float velocity = yoffset * MouseSensitivity * 0.25f;
    Position += Front * velocity;
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= yoffset;
    if (Zoom < 1.0f) Zoom = 1.0f;
    if (Zoom > 45.0f) Zoom = 45.0f;
}
void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // First calculate the base Right and Up vectors (before roll)
    glm::vec3 baseRight = glm::normalize(glm::cross(Front, WorldUp));
    glm::vec3 baseUp = glm::normalize(glm::cross(baseRight, Front));

    if (Roll != 0.0f) {
        // Create rotation matrix around Front vector for roll
        float rollRad = glm::radians(Roll);
        glm::mat4 rollMatrix = glm::rotate(glm::mat4(1.0f), rollRad, Front);

        // Apply roll rotation to the base vectors
        Right = glm::vec3(rollMatrix * glm::vec4(baseRight, 0.0f));
        Up = glm::vec3(rollMatrix * glm::vec4(baseUp, 0.0f));
    } else {
        Right = baseRight;
        Up = baseUp;
    }
}
