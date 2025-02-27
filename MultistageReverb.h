#pragma once
#include <array>
#include <utility>
#include "MultistageReverbConfig.h"  // Defines StageConfig and MultistageReverbConfig
#include "StageReverb.h"             // Defines StageReverb

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb Class 
        // (The full reverb network is computed at compile time via unrolled stage chains 
        // and compile-time computed routing via a constexpr matrix.)
        //--------------------------------------------------------------------------
        class MultistageReverb {
        public:
            static constexpr size_t numStages = MultistageReverbConfig::stages.size();

            MultistageReverb() = default;

            void prepare(float sampleRate) {
                prepareStages(sampleRate, std::make_index_sequence<numStages>{});
            }
            void reset() {
                resetStages(std::make_index_sequence<numStages>{});
            }
            JUCE_FORCEINLINE float processSample(float input) {
                return processSampleImpl(input, std::make_index_sequence<numStages>{});
            }

        private:
            std::array<StageReverb<StageConfig>, numStages> stages;

            template <size_t... Is>
            JUCE_FORCEINLINE void prepareStages(float sr, std::index_sequence<Is...>) {
                ((stages[Is].prepare(sr)), ...);
            }
            template <size_t... Is>
            JUCE_FORCEINLINE void resetStages(std::index_sequence<Is...>) {
                ((stages[Is].reset()), ...);
            }
            template <size_t... Is>
            JUCE_FORCEINLINE float processSampleImpl(float input, std::index_sequence<Is...>) {
                // For each stage, multiply the stage’s output by the compile-time computed row sum.
                return (... + (stages[Is].processSample(input) * rowSum<Is>()));
            }
            template <size_t I, size_t... Js>
            static constexpr float rowSumImpl(std::index_sequence<Js...>) {
                return (... + MultistageReverbConfig::routingMatrix[I][Js]);
            }
            template <size_t I>
            static constexpr float rowSum() {
                return rowSumImpl<I>(std::make_index_sequence<numStages>{});
            }
        };

    } // namespace multistage
} // namespace project
