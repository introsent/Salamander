#include "camera.h"

// Сonstructor: look-at target
Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 worldUp)
    : Position(position), WorldUp(worldUp) {
    // Compute initial Front vector
    Front = glm::normalize(target - position);
    // Derive Euler angles from Front
    Pitch = glm::degrees(asin(Front.z));
    Yaw   = -glm::degrees(atan2(Front.y, Front.x));
    updateCameraVectors();
}

// Constructor: custom yaw/pitch
Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch)
    : Position(position), WorldUp(worldUp), Yaw(yaw), Pitch(pitch) {
    updateCameraVectors();
}
glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, WorldUp);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    glm::mat4 proj = glm::perspective(glm::radians(Zoom), aspectRatio, 0.1f, 10.0f);
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

    Yaw += xoffset;
    Pitch += yoffset;

    if (Pitch > 89.0f) Pitch = 89.0f;
    if (Pitch < -89.0f) Pitch = -89.0f;

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
    // Recalculate Front from Euler angles
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = -sin(glm::radians(Yaw)) * cos(glm::radians(Pitch)); // Yaw affects X/Y plane
    front.z = sin(glm::radians(Pitch));
    Front = glm::normalize(front);
    // Recalculate Right and Up
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}