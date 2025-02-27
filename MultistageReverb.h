#pragma once
#include <array>
#include <utility>
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

            // Prepare all stages.
            void prepare(float sampleRate) {
                prepareStages(sampleRate, std::make_index_sequence<numStages>{});
            }

            // Reset all stages.
            void reset() {
                resetStages(std::make_index_sequence<numStages>{});
            }

            // processSample: Fully inlined chain combining stage processing
            // and routing matrix multiplications without temporary arrays.
            JUCE_FORCEINLINE float processSample(float input) {
                return processSampleImpl(input, std::make_index_sequence<numStages>{});
            }

        private:
            // Use the compile-time StageConfig type (all stages share the same type).
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

            // --- Optimized Process Sample Implementation ---
            // This function unrolls over each stage, computes its output (which itself
            // inlines LFO update and AP processing) and multiplies it by the compile-time
            // computed sum of its routing matrix row. The final output is the sum over stages.
            template <size_t... Is>
            JUCE_FORCEINLINE float processSampleImpl(float input, std::index_sequence<Is...>) {
                return (... + (stages[Is].processSample(input) * rowSum<Is>()));
            }

            // Compute the sum of the routing matrix row for stage I.
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
