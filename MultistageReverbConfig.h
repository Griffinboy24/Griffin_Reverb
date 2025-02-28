#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h" // Contains ms_make_array and common DSP classes

namespace project {
    namespace multistage {

        // Define three distinct stage configuration types.
        struct StageConfig0 {
            inline static constexpr auto lfoFrequencies = ms_make_array(1.1341f);
            // Removed post-delay parameter.
            struct AP {
                float baseDelay;      // in samples (at 44100 Hz)
                float coefficient;
                float depth;          // in samples (at 44100 Hz)
                size_t lfoIndex;
                constexpr float maxDelay() const { return baseDelay + 50.f; }
            };
            inline static constexpr auto aps = ms_make_array(
                AP{ 80.0f, 0.55f, 8.0f, 0 },
                AP{ 120.0f, 0.55f, 8.0f, 0 },
                AP{ 200.0f, 0.55f, 8.0f, 0 },
                AP{ 280.0f, 0.55f, 8.0f, 0 },
                AP{ 440.0f, 0.55f, 8.0f, 0 }
            );
        };

        struct StageConfig1 {
            inline static constexpr auto lfoFrequencies = ms_make_array(0.9128f, 1.1341f, 1.0f);
            // Removed post-delay parameter.
            struct AP {
                float baseDelay;      // in samples (at 44100 Hz)
                float coefficient;
                float depth;          // in samples (at 44100 Hz)
                size_t lfoIndex;
                constexpr float maxDelay() const { return baseDelay + 50.f; }
            };
            inline static constexpr auto aps = ms_make_array(
                AP{ 1200.0f, 0.65f, 10.0f, 2 },
                AP{ 1400.0f, 0.63f, 9.0f, 1 },
                AP{ 1600.0f, 0.61f, 11.0f, 0 },
                AP{ 1800.0f, 0.59f, 10.0f, 2 },
                AP{ 2000.0f, 0.57f, 9.0f, 1 }
            );
        };

        struct StageConfig2 {
            inline static constexpr auto lfoFrequencies = ms_make_array(0.1f);
            // Removed post-delay parameter.
            struct AP {
                float baseDelay;      // in samples (at 44100 Hz)
                float coefficient;
                float depth;          // in samples (at 44100 Hz)
                size_t lfoIndex;
                constexpr float maxDelay() const { return baseDelay + 50.f; }
            };
            inline static constexpr auto aps = ms_make_array(
                AP{ 100.0f, 0.0f, 0.0f, 0 }
            );
        };

        // Wrap configuration constants in a struct.
        struct MultistageReverbConfig {
            using StageTuple = std::tuple<StageConfig0, StageConfig1, StageConfig2>;
            inline static constexpr StageTuple stages = StageTuple{ StageConfig0{}, StageConfig1{}, StageConfig2{} };
            static constexpr size_t NumStages = std::tuple_size<StageTuple>::value;
            static constexpr size_t NumNodes = NumStages + 2;  // Input + stages + output.
            inline static constexpr size_t InputIndex = 0;
            inline static constexpr size_t FirstStageIndex = 1;
            inline static constexpr size_t OutputIndex = NumNodes - 1;
            inline static constexpr std::array<std::array<float, NumNodes>, NumNodes> routingMatrix = { {
                { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
                { 0.0f, 0.0f, 0.7f, 0.0f, 0.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }
            } };
        };

    } // namespace multistage
} // namespace project
