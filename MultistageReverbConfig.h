#pragma once
#include <array>
#include "ReverbCommon.h" // Contains ms_make_array and common DSP classes

namespace project {
    namespace multistage {

        //==============================================================================
        // Stage Configuration for a Single Reverb Stage
        //==============================================================================
        struct StageConfig {
            // LFO speeds (in Hz) for this stage.
            // Modify the array literal to change or add LFO frequencies.
            inline static constexpr auto lfoFrequencies = ms_make_array(1.5f); // Example: one LFO

            // Allpass (AP) definition for this stage.
            // Each AP is defined by its base delay (ms), coefficient, modulation depth, and LFO index.
            struct AP {
                float baseDelay;
                float coefficient;
                float depth;
                size_t lfoIndex;
                constexpr float maxDelay() const { return baseDelay + 50.f; }
            };

            // List the APs for this stage.
            // Modify the array literal to add or change AP definitions.
            inline static constexpr auto aps = ms_make_array(
                AP{ 10.f, 0.5f, 0.3f, 0 }
            );
        };

        //==============================================================================
        // Multistage Reverb Configuration
        //==============================================================================
        struct MultistageReverbConfig {
            // Define an array of StageConfig objects, one per stage.
            // For example, here we define two identical stages.
            inline static constexpr std::array<StageConfig, 2> stages = { StageConfig{}, StageConfig{} };

            // Define a compile?time 2D routing matrix.
            // Each element [src][dst] defines the gain from stage "src" (send) to stage "dst" (receive).
            // Modify the matrix below to adjust the routing.
            inline static constexpr std::array<std::array<float, stages.size()>, stages.size()> routingMatrix = { {
    { 1.0f, 0.0f },  // Stage 0 sends only to Stage 0
    { 0.0f, 1.0f }   // Stage 1 sends only to Stage 1
} };




        };

    } // namespace multistage
} // namespace project
