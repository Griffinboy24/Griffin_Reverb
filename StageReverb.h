#pragma once
#include <array>
#include "ReverbCommon.h"  // Contains ms_make_array, SimpleLFO, SimpleAP

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------------
        // StageReverb Class Template
        //  - We no longer store or update any LFOs inside the stage. Instead,
        //    each allpass filter simply uses its 'lfoIndex' to fetch the global
        //    LFO value from a shared pointer (set by MultistageReverb).
        //--------------------------------------------------------------------------------
        template <typename StageConfig>
        class StageReverb
        {
        public:
            // We used to have: numLFOs, lfos[], etc. That is now removed.
            static constexpr size_t numAPs = std::tuple_size<decltype(StageConfig::aps)>::value;

            StageReverb() {
                initAPs(std::make_index_sequence<numAPs>{});
                globalLfoPtr = nullptr;
            }

            // Called once from outside
            void prepare(float sampleRate) {
                prepareAPs(sampleRate, std::make_index_sequence<numAPs>{});
            }

            // Called once from outside
            void reset() {
                resetAPs(std::make_index_sequence<numAPs>{});
            }

            // Set pointer to the global LFO-output array
            void setGlobalLfoOutputsPointer(const float* ptr) {
                globalLfoPtr = ptr;
            }

            // processSample: pass the node input through the chain of APs,
            // each of which uses globalLfoPtr[lfoIndex] to modulate.
            JUCE_FORCEINLINE float processSample(float nodeInput)
            {
                return processAPsRecursive<0>(nodeInput);
            }

        private:
            std::array<project::SimpleAP, numAPs> aps;
            const float* globalLfoPtr;  // Points to the shared LFO array in MultistageReverb

            //--- AP Initialization ---
            template <size_t... Is>
            JUCE_FORCEINLINE void initAPs(std::index_sequence<Is...>) {
                ((aps[Is] = project::SimpleAP(
                    StageConfig::aps[Is].baseDelay,
                    StageConfig::aps[Is].coefficient,
                    StageConfig::aps[Is].depth,
                    StageConfig::aps[Is].lfoIndex)), ...);
            }

            //--- Prepare ---
            template <size_t... Is>
            JUCE_FORCEINLINE void prepareAPs(float sr, std::index_sequence<Is...>) {
                ((aps[Is].prepare(sr)), ...);
            }

            //--- Reset ---
            template <size_t... Is>
            JUCE_FORCEINLINE void resetAPs(std::index_sequence<Is...>) {
                ((aps[Is].reset()), ...);
            }

            //--- Recursive chain of APs ---
            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if < I < numAPs, float>::type
                processAPsRecursive(float currentSignal)
            {
                const size_t index = aps[I].getLfoIndex();
                float modValue = (globalLfoPtr != nullptr) ? globalLfoPtr[index] : 0.0f;
                float next = aps[I].processSample(currentSignal, modValue);
                return processAPsRecursive<I + 1>(next);
            }

            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if<I == numAPs, float>::type
                processAPsRecursive(float currentSignal)
            {
                return currentSignal;
            }
        };

    } // namespace multistage
} // namespace project
