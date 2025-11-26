#include <iostream>
#include "application.h"

#define STB_IMAGE_IMPLEMENTATION
int main() {
    VulkanApplication app;
    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
