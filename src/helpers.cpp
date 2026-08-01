//
// Created by ivans on 01/08/2026.
//

#include "helpers.h"

#include <iostream>
#include <queue>

//https://www.geeksforgeeks.org/cpp/kahns-algorithm-in-cpp/
std::vector<int> Salamander::Helpers::topologicalSort(std::vector<std::vector<uint32_t>>& adj, int V) {
    // Vector to store indegree of each vertex
    std::vector<int> indegree(V);

    // Calculating indegree for each vertex
    for (int i = 0; i < V; i++) {
        for (auto it : adj[i]) {
            indegree[it]++;
        }
    }

    // Queue to store vertices with indegree 0
    std::queue<int> q;

    // Pushing vertices with indegree 0 to the queue
    for (int i = 0; i < V; i++) {
        if (indegree[i] == 0) {
            q.push(i);
        }
    }

    // Vector to store the result
    std::vector<int> result;

    // Performing topological sort
    while (!q.empty()) {
        // Get the front element from the queue
        int node = q.front();
        q.pop();

        // Add it to the result
        result.push_back(node);

        // Decrease indegree of adjacent vertices as the
        // current node is in topological order
        for (auto it : adj[node]) {
            indegree[it]--;

            // If indegree becomes 0, push it to the queue
            if (indegree[it] == 0) {
                q.push(it);
            }
        }
    }

    // Check for cycle
    if (result.size() != V) {
        // If result size is not equal to number of
        // vertices, graph contains a cycle
        std::cout << "[Helpers] Graph contains cycle!" << std::endl;
        return {};
    }

    // Return the topological order
    return result;
}
