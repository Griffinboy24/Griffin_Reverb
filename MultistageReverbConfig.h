#pragma once
#include <array>
#include "ReverbCommon.h" // Contains ms_make_array and common DSP classes

namespace project {
    namespace multistage {

        //==============================================================================
        // StageConfig: Configuration for a single reverb stage.
        // The parameters (LFO frequencies and Allpass Delay Line (AP) definitions) are constexpr.
        // Note: The baseDelay and depth values are specified in samples (at 44100 Hz) and will be converted to ms internally.
        //==============================================================================
        struct StageConfig {
            // LFO frequencies for modulation in this stage.
            inline static constexpr auto lfoFrequencies = ms_make_array(1.0f);
            // AP struct holds parameters for a simple allpass delay line.
            struct AP {
                float baseDelay;      // in samples (at 44100 Hz)
                float coefficient;
                float depth;          // in samples (at 44100 Hz)
                size_t lfoIndex;
                constexpr float maxDelay() const { return baseDelay + 50.f; }
            };
            // Array of AP configurations.
            inline static constexpr auto aps = ms_make_array(
                AP{ 100.0f, 0.7f, 10.0f, 0 } // For example, 100 samples delay 
            );
        };

        //==============================================================================
        // MultistageReverbConfig: Top-level configuration for the reverb network.
        // Defines both the stages and the compile-time routing matrix.
        //==============================================================================
        struct MultistageReverbConfig {
            inline static constexpr std::array<StageConfig, 2> stages = { StageConfig{}, StageConfig{} };
            inline static constexpr size_t NumNodes = stages.size() + 2;
            inline static constexpr size_t InputIndex = 0;
            inline static constexpr size_t FirstStageIndex = 1;
            inline static constexpr size_t OutputIndex = NumNodes - 1;

            /*
                       |  In  |  S0  |  S1  |  Out
                  ---------------------------------
                  In   |  0   |  0   |  0   |  0.0
                  S0   |  0   |  0   |  0   |  0.0
                  S1   |  0   |  0   |  0   |  0.0
                  Out  |  0   |  0   |  0   |  0.0
            */
            // The routing matrix is defined such that:
            //   - The Input (row 0) is routed entirely to S0.
            //   - S0 is routed entirely to the Output.
            //   - Other nodes have no contributions.
            inline static constexpr std::array<std::array<float, NumNodes>, NumNodes> routingMatrix = { {
                    { 0.0f,  1.0f,  0.0f,  0.0f },
                    { 0.0f,  0.0f,  1.0f,  0.0f },
                    { 0.0f,  0.0f,  0.0f,  1.0f },
                    { 0.0f,  0.0f,  0.0f,  0.0f }
                } };
        };

    } // namespace multistage
} // namespace project
