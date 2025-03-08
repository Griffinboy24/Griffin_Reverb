#pragma once
#include <array>

namespace project {
    namespace multistage {

        // Example Routing Provider
        // The user can define their own version of this for different networks.
        struct MyRoutingProvider
        {
            // Called by MultiStageReverb at compile time to get the matrix
            template <size_t NumNodes>
            static constexpr auto makeMatrix()
            {
                // We build a NumNodes x NumNodes array, all zero
                std::array<std::array<float, NumNodes>, NumNodes> mat = {};

                // For demonstration, do a simple chain:
                // node0 -> node1 -> node2 -> ... -> node(NumNodes-1)
                for (size_t i = 0; i < NumNodes - 1; ++i) {
                    mat[i][i + 1] = 1.f; // feed node i to node i+1
                }

                return mat;
            }
        };

    } // namespace multistage
} // namespace project
