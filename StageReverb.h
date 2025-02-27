#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h"  // Contains ms_make_array, SimpleLFO, and SimpleAP

namespace project {
    namespace multistage {

        //---------------------------------------------------------------------- 
        // StageReverb Class Template
        // (Fully unrolled via index sequences so that LFO updates, AP processing, 
        // and the stage routing (via compile-time constants) are bound at compile time.)
        //---------------------------------------------------------------------- 
        template <typename StageConfig>
        class StageReverb {
        public:
            static constexpr size_t numLFOs = std::tuple_size<decltype(StageConfig::lfoFrequencies)>::value;
            static constexpr size_t numAPs = std::tuple_size<decltype(StageConfig::aps)>::value;

            // Constructor: Unroll LFO and AP initialization at compile time.
            StageReverb() {
                initLFOs(std::make_index_sequence<numLFOs>{});
                initAPs(std::make_index_sequence<numAPs>{});
            }

            void prepare(float sampleRate) {
                prepareLFOs(sampleRate, std::make_index_sequence<numLFOs>{});
                prepareAPs(sampleRate, std::make_index_sequence<numAPs>{});
            }

            void reset() {
                resetAPs(std::make_index_sequence<numAPs>{});
            }

            // processSample: Update LFOs and process all APs in a fully inlined chain.
            JUCE_FORCEINLINE float processSample(float input) {
                std::array<float, numLFOs> lfoValues{};
                updateLFOs(lfoValues, std::make_index_sequence<numLFOs>{});
                return processAPs<0>(input, lfoValues);
            }

        private:
            std::array<SimpleLFO, numLFOs> lfos;
            std::array<SimpleAP, numAPs> aps;

            // --- Initialization Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void initLFOs(std::index_sequence<Is...>) {
                ((lfos[Is] = SimpleLFO(StageConfig::lfoFrequencies[Is])), ...);
            }
            template <size_t... Is>
            JUCE_FORCEINLINE void initAPs(std::index_sequence<Is...>) {
                ((aps[Is] = SimpleAP(StageConfig::aps[Is].baseDelay,
                    StageConfig::aps[Is].coefficient,
                    StageConfig::aps[Is].depth,
                    StageConfig::aps[Is].lfoIndex)), ...);
            }

            // --- Prepare Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void prepareLFOs(float sr, std::index_sequence<Is...>) {
                ((lfos[Is].prepare(sr)), ...);
            }
            template <size_t... Is>
            JUCE_FORCEINLINE void prepareAPs(float sr, std::index_sequence<Is...>) {
                ((aps[Is].prepare(sr)), ...);
            }

            // --- Reset Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void resetAPs(std::index_sequence<Is...>) {
                ((aps[Is].reset()), ...);
            }

            // --- Process Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void updateLFOs(std::array<float, numLFOs>& lfoValues, std::index_sequence<Is...>) {
                ((lfoValues[Is] = lfos[Is].update()), ...);
            }

            // Recursive unrolling of AP processing.
            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if<I == numAPs, float>::type
                processAPs(float output, const std::array<float, numLFOs>&) {
                return output;
            }
            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if < I < numAPs, float>::type
                processAPs(float output, const std::array<float, numLFOs>& lfoValues) {
                constexpr size_t cfgLfoIdx = StageConfig::aps[I].lfoIndex;
                constexpr size_t useIdx = (cfgLfoIdx < numLFOs ? cfgLfoIdx : 0);
                float newOutput = aps[I].processSample(output, lfoValues[useIdx]);
                return processAPs<I + 1>(newOutput, lfoValues);
            }
        };

    } // namespace multistage
} // namespace project
