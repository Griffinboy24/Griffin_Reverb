#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h" // Contains ms_make_array and common DSP classes

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------------
        // Combined MultistageReverbConfig
        //   - Holds global LFO definitions directly in the struct.
        //   - Also contains the stage definitions referencing those LFO indices.
        //--------------------------------------------------------------------------------
        struct MultistageReverbConfig
        {
            // ***** Put the LFO definitions as static members of this struct. *****
            inline static constexpr auto globalLfoFrequencies = ms_make_array(
                0.1f,      // index=0
                0.9128f,   // index=1
                1.1341f    // index=2
            );
            inline static constexpr size_t NumGlobalLFOs = globalLfoFrequencies.size();

            //================== Stage Definitions ==================//
            struct StageConfig0
            {
                struct AP {
                    float baseDelay;
                    float coefficient;
                    float depth;
                    size_t lfoIndex;
                    constexpr float maxDelay() const { return baseDelay + 50.f; }
                };
                // All use lfoIndex=2 => frequency=1.1341
                inline static constexpr auto aps = ms_make_array(
                    AP{ 80.0f, 0.55f,  8.0f, 2 },
                    AP{ 120.0f, 0.55f,  8.0f, 2 },
                    AP{ 200.0f, 0.55f,  8.0f, 2 },
                    AP{ 280.0f, 0.55f,  8.0f, 2 },
                    AP{ 440.0f, 0.55f,  8.0f, 2 }
                );
            };

            struct StageConfig1
            {
                struct AP {
                    float baseDelay;
                    float coefficient;
                    float depth;
                    size_t lfoIndex;
                    constexpr float maxDelay() const { return baseDelay + 50.f; }
                };
                // Indices: 0 => 0.1f, 1 => 0.9128f, 2 => 1.1341f
                inline static constexpr auto aps = ms_make_array(
                    AP{ 1200.0f, 0.65f, 10.0f, 2 },
                    AP{ 1400.0f, 0.63f,  9.0f, 0 },
                    AP{ 1600.0f, 0.61f, 11.0f, 1 },
                    AP{ 1800.0f, 0.59f, 10.0f, 2 },
                    AP{ 2000.0f, 0.57f,  9.0f, 0 }
                );
            };

            struct StageConfig2
            {
                struct AP {
                    float baseDelay;
                    float coefficient;
                    float depth;
                    size_t lfoIndex;
                    constexpr float maxDelay() const { return baseDelay + 50.f; }
                };
                // Single AP uses index=0 => frequency=0.1
                inline static constexpr auto aps = ms_make_array(
                    AP{ 100.0f, 0.0f, 0.0f, 0 }
                );
            };

            // Combine them in a tuple
            using StageTuple = std::tuple<StageConfig0, StageConfig1, StageConfig2>;
            inline static constexpr StageTuple stages = { StageConfig0{}, StageConfig1{}, StageConfig2{} };

            // Number of stages = 3, number of nodes = stages + 2 (input + output)
            static constexpr size_t NumStages = std::tuple_size<StageTuple>::value;
            static constexpr size_t NumNodes = NumStages + 2;

            // Node routing indices
            inline static constexpr size_t InputIndex = 0;
            inline static constexpr size_t FirstStageIndex = 1;
            inline static constexpr size_t OutputIndex = NumNodes - 1;

            // Node routing matrix
            inline static constexpr std::array<std::array<float, NumNodes>, NumNodes> routingMatrix =
            { {
                { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
                { 0.0f, 0.0f, 0.7f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
            } };
        };

    } // namespace multistage
} // namespace project
