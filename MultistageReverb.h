#pragma once
#include <array>
#include "MultistageReverbConfig.h"  // Defines project::multistage::StageConfig and MultistageReverbConfig
#include "StageReverb.h"             // Defines project::multistage::StageReverb

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb Class (Optimized with compile-time unrolling)
        //--------------------------------------------------------------------------
        class MultistageReverb {
        public:
            static constexpr size_t numStages = MultistageReverbConfig::stages.size();

            MultistageReverb() = default;

            // prepare: Unrolled call to prepare on each stage.
            void prepare(float sampleRate) {
                prepareStages(sampleRate, std::make_index_sequence<numStages>{});
            }

            // reset: Unrolled call to reset on each stage.
            void reset() {
                resetStages(std::make_index_sequence<numStages>{});
            }

            // processSample: Unrolled stage processing, routing, and final summing.
            JUCE_FORCEINLINE float processSample(float input) {
                std::array<float, numStages> stageOutputs{};
                computeStageOutputs(input, stageOutputs, std::make_index_sequence<numStages>{});
                auto routedInputs = computeRoutedInputs(stageOutputs, std::make_index_sequence<numStages>{});
                return sumArray(routedInputs, std::make_index_sequence<numStages>{});
            }

        private:
            std::array<StageReverb<StageConfig>, numStages> stages;

            // --- Prepare Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void prepareStages(float sr, std::index_sequence<Is...>) {
                ((stages[Is].prepare(sr)), ...);
            }

            template <size_t... Is>
            JUCE_FORCEINLINE void resetStages(std::index_sequence<Is...>) {
                ((stages[Is].reset()), ...);
            }

            // --- Stage Processing Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void computeStageOutputs(float input, std::array<float, numStages>& outputs, std::index_sequence<Is...>) {
                ((outputs[Is] = stages[Is].processSample(input)), ...);
            }

            // For each destination stage, compute the routed input by summing contributions from all sources.
            template <size_t Dest, size_t... Srcs>
            JUCE_FORCEINLINE float computeRoutedInputForDest(const std::array<float, numStages>& stageOutputs, std::index_sequence<Srcs...>) {
                return ((MultistageReverbConfig::routingMatrix[Srcs][Dest] * stageOutputs[Srcs]) + ...);
            }

            template <size_t... Dests>
            JUCE_FORCEINLINE std::array<float, numStages> computeRoutedInputs(const std::array<float, numStages>& stageOutputs, std::index_sequence<Dests...>) {
                return { computeRoutedInputForDest<Dests>(stageOutputs, std::make_index_sequence<numStages>{})... };
            }

            // Final summing of routed inputs.
            template <size_t... Is>
            JUCE_FORCEINLINE float sumArray(const std::array<float, numStages>& arr, std::index_sequence<Is...>) {
                return (... + arr[Is]);
            }
        };

    } // namespace multistage
} // namespace project
