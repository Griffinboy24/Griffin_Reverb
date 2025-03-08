#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h"

// To do:  
// 2) Allow routing to unique stereo stages as final outputs
// 3) Create svf filter stages (routable) 
// 4) Improve routing by defining a list-of-connections instead of a matrix
// 5) Higher order (nested) allpass types support 
// 6) FDN support + classic multichannel matrix types built in

namespace project {
    namespace multistage {

        // Define a connection structure.
        struct Connection {
            size_t src;         // Source node index.
            size_t dst;         // Destination node index.
            float baseWeight;   // Base connection weight.
            bool scaleFeedback; // If true, weight is scaled by a runtime feedback parameter.
        };

        struct MyReverbConfig
        {
            // LFO definitions.
            static constexpr size_t NumGlobalLFOs = 3;
            inline static constexpr auto lfoFrequencies = ms_make_array(0.9128f, 1.1341f, 1.0f);
            inline static constexpr auto lfoAmplitudes = ms_make_array(11.0f, 9.0f, 10.0f);

            // Stage0 
            struct StageConfig0 {
                static constexpr bool scaleDelay = false; // Delay times scaling
                static constexpr bool scaleCoeff = true;   // Coefficient scaling 
                struct AP {
                    float baseDelay;
                    float coefficient;
                    size_t lfoIndex;
                };
                inline static constexpr auto aps = ms_make_array(
                    AP{ 80.0f, 1.0f, 1 },
                    AP{ 120.0f, 1.0f, 2 },
                    AP{ 200.0f, 1.0f, 0 },
                    AP{ 280.0f, 1.0f, 1 },
                    AP{ 440.0f, 1.0f, 2 }
                );
            };

            // Stage1
            struct StageConfig1 {
                static constexpr bool scaleDelay = true;
                static constexpr bool scaleCoeff = true; 
                struct AP {
                    float baseDelay;
                    float coefficient;
                    size_t lfoIndex;
                };
                inline static constexpr auto aps = ms_make_array(
                    AP{ 300.0f, 0.9f, 0 },
                    AP{ 700.0f, 0.9f, 1 },
                    AP{ 1100.0f, 0.9f, 2 },
                    AP{ 1900.0f, 0.9f, 0 },
                    AP{ 2300.0f, 0.9f, 1 },
                    AP{ 2900.0f, 0.9f, 2 }
                );
            };

            // Stage2 
            struct StageConfig2 {
                static constexpr bool scaleDelay = true; 
                static constexpr bool scaleCoeff = true;  
                struct AP {
                    float baseDelay;
                    float coefficient;
                    size_t lfoIndex;
                };
                inline static constexpr auto aps = ms_make_array(
                    AP{ 400.0f, 0.9f, 2 },
                    AP{ 600.0f, 0.9f, 1 },
                    AP{ 1000.0f, 0.9f, 0 },
                    AP{ 1400.0f, 0.9f, 2 },
                    AP{ 2200.0f, 0.9f, 1 }
                );
            };

            // Combine: 3 stages => 5 nodes (node 0: input, nodes 1-3: stages, node 4: output)
            using StageTuple = std::tuple<StageConfig0, StageConfig1, StageConfig2>;
            inline static constexpr StageTuple stages = { StageConfig0{}, StageConfig1{}, StageConfig2{} };

            static constexpr size_t NumStages = std::tuple_size<StageTuple>::value;
            static constexpr size_t NumNodes = NumStages + 2;

            // Instead of a routing matrix, define a list-of-connections.
            // source, send, amount, connect to feedback parameter
            inline static constexpr std::array<Connection, 6> connections = { {
                { 0, 1, 1.0f, false },
                { 1, 2, 1.0f, false },
                { 1, 3, 1.0f, false },
                { 2, 2, 0.9f, true },   // Feedback connection.
                { 2, 4, 1.0f, false },
                { 3, 4, 0.8f, false }
            } };
        };

    } // namespace multistage
} // namespace project
