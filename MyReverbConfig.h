#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h"

namespace project {
    namespace multistage {

        struct MyReverbConfig
        {
            // LFO definitions
            static constexpr size_t NumGlobalLFOs = 3;
            inline static constexpr auto lfoFrequencies = ms_make_array(0.9128f, 1.1341, 1.0f);
            inline static constexpr auto lfoAmplitudes = ms_make_array(11.0f, 9.0f, 10.0f);

            // Stage0 
            struct StageConfig0 {
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

            
            /*
         |  In  |  S0  |  S1  |  S2  |  Out
-----------------------------------------------
In       | 0.0  | 1.0  | 0.0  | 0.0  | 0.0
S0       | 0.0  | 0.0  | 0.0  | 0.0  | 1.0
S1       | 0.0  | 0.0  | 0.0  | 0.0  | 0.0
S2       | 0.0  | 0.0  | 0.0  | 0.0  | 0.0
Out      | 0.0  | 0.0  | 0.0  | 0.0  | 0.0



                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
            


*/


            // 5x5 routing matrix
            inline static constexpr std::array<std::array<float, NumNodes>, NumNodes> routingMatrix =
            { {
                { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f },
                { 0.0f, 0.0f, 0.7f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
            } };
        };

    } // namespace multistage
} // namespace project
