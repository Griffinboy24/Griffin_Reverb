#pragma once
#include <array>
#include "ReverbCommon.h" // Contains ms_make_array and common DSP classes

namespace project {
    namespace multistage {

        //==============================================================================
        // StageConfig: Configuration for a single reverb stage.
        // The parameters (LFO frequencies and AP definitions) are all constexpr.
        //==============================================================================
        struct StageConfig {
            inline static constexpr auto lfoFrequencies = ms_make_array(1.5f); // One LFO for this stage.
            struct AP {
                float baseDelay;
                float coefficient;
                float depth;
                size_t lfoIndex;
                constexpr float maxDelay() const { return baseDelay + 50.f; }
            };
            inline static constexpr auto aps = ms_make_array(
                AP{ 10.f, 0.5f, 0.3f, 0 }
            );
        };

        //==============================================================================
        // MultistageReverbConfig: Top-level configuration for the entire reverb network.
        // The stages and inter-stage routing matrix are defined at compile time.
        //==============================================================================
        struct MultistageReverbConfig {
            inline static constexpr std::array<StageConfig, 2> stages = { StageConfig{}, StageConfig{} };
            inline static constexpr std::array<std::array<float, 2>, 2> routingMatrix = { {{1.0f, 0.0f}, {0.0f, 1.0f}} };
        };

    } // namespace multistage
} // namespace project
