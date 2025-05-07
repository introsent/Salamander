#include <iostream>
#include "application.h"
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
