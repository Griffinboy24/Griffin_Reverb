#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h"

// To do:  
// 1) Allow for global user parameters on delay length and coefficients and feedback 
// 2) Allow routing to unique stereo stages as final outputs
// 3) Create svf filter stages (routable) 
// 4) Is there a cleverer way to define routing? Make it easier for different stage types. Perhaps not a matrix but a list of connections? Increase Freedom. 
// 5) Higher order (nested) allpass types support 
// 6) FDN support + classic multichannel matrix types built in

namespace project {
    namespace multistage {

        struct MyReverbConfig
        {
            // LFO definitions
            static constexpr size_t NumGlobalLFOs = 3;
            inline static constexpr auto lfoFrequencies = ms_make_array(0.9128f, 1.1341f, 1.0f);
            inline static constexpr auto lfoAmplitudes = ms_make_array(11.0f, 9.0f, 10.0f);

            // Stage0 
            struct StageConfig0 {
                static constexpr bool scaleDelay = false; // Delay times will be scaled.
                struct AP {
                    float baseDelay;
                    float coefficient;
                    size_t lfoIndex;
                };
                inline static constexpr auto aps = ms_make_array(
                    AP{ 80.0f, 0.5f, 1 },
                    AP{ 120.0f, 0.5f, 2 },
                    AP{ 200.0f, 0.5f, 0 },
                    AP{ 280.0f, 0.5f, 1 },
                    AP{ 440.0f, 0.5f, 2 }
                );
            };

            // Stage1
            struct StageConfig1 {
                static constexpr bool scaleDelay = true;  // Delay times will be scaled.
                struct AP {
                    float baseDelay;
                    float coefficient;
                    size_t lfoIndex;
                };
                inline static constexpr auto aps = ms_make_array(
                    AP{ 300.0f, 0.5f, 0 },
                    AP{ 700.0f, 0.5f, 1 },
                    AP{ 1100.0f, 0.5f, 2 },
                    AP{ 1900.0f, 0.5f, 0 },
                    AP{ 2300.0f, 0.5f, 1 },
                    AP{ 2900.0f, 0.5f, 2 }
                );
            };

            // Stage2 
            struct StageConfig2 {
                static constexpr bool scaleDelay = true; // Delay times will be scaled.
                struct AP {
                    float baseDelay;
                    float coefficient;
                    size_t lfoIndex;
                };
                inline static constexpr auto aps = ms_make_array(
                    AP{ 400.0f, 0.5f, 2 },
                    AP{ 600.0f, 0.5f, 1 },
                    AP{ 1000.0f, 0.5f, 0 },
                    AP{ 1400.0f, 0.5f, 2 },
                    AP{ 2200.0f, 0.5f, 1 }
                );
            };

            // Combine => 3 stages => 5 nodes (input + 3 stages + output)
            using StageTuple = std::tuple<StageConfig0, StageConfig1, StageConfig2>;
            inline static constexpr StageTuple stages = { StageConfig0{}, StageConfig1{}, StageConfig2{} };

            static constexpr size_t NumStages = std::tuple_size<StageTuple>::value;
            static constexpr size_t NumNodes = NumStages + 2;

            // 5x5 routing matrix
            inline static constexpr std::array<std::array<float, NumNodes>, NumNodes> routingMatrix =
            { {
                { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f },
                { 0.0f, 0.0f, 0.7f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
            } };
        };

    } // namespace multistage
} // namespace project
