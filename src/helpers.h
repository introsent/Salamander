//
// Created by ivans on 01/08/2026.
//

#ifndef SALAMANDER_HELPERS_H
#define SALAMANDER_HELPERS_H
#include <vector>

namespace Salamander {
    struct Helpers {
        //https://www.geeksforgeeks.org/cpp/kahns-algorithm-in-cpp/
        static std::vector<int> topologicalSort(std::vector<std::vector<uint32_t>>& adj,int V);
    };
}



#endif //SALAMANDER_HELPERS_H