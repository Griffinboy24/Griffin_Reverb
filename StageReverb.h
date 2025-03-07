#pragma once
#include <array>
#include "ReverbCommon.h"

namespace project {
    namespace multistage {

        template <typename StageConfig>
        class StageReverb
        {
        public:
            static constexpr size_t numAPs = std::tuple_size<decltype(StageConfig::aps)>::value;

            StageReverb()
            {
                initAPs(std::make_index_sequence<numAPs>{});
                globalLfoPtr = nullptr;
            }

            void prepare(float sampleRate)
            {
                prepareAPs(sampleRate, std::make_index_sequence<numAPs>{});
            }

            void reset()
            {
                resetAPs(std::make_index_sequence<numAPs>{});
            }

            // Called externally so we know where to read the updated LFO values
            void setGlobalLfoOutputsPointer(const float* ptr)
            {
                globalLfoPtr = ptr;
            }

            JUCE_FORCEINLINE float processSample(float nodeInput)
            {
                return processAPsRecursive<0>(nodeInput);
            }

        private:
            std::array<project::SimpleAP, numAPs> aps;
            const float* globalLfoPtr;

            template <size_t... Is>
            JUCE_FORCEINLINE void initAPs(std::index_sequence<Is...>)
            {
                ((aps[Is] = project::SimpleAP(
                    StageConfig::aps[Is].baseDelay,
                    StageConfig::aps[Is].coefficient,
                    StageConfig::aps[Is].depth,
                    StageConfig::aps[Is].lfoIndex)), ...);
            }

            template <size_t... Is>
            JUCE_FORCEINLINE void prepareAPs(float sr, std::index_sequence<Is...>)
            {
                ((aps[Is].prepare(sr)), ...);
            }

            template <size_t... Is>
            JUCE_FORCEINLINE void resetAPs(std::index_sequence<Is...>)
            {
                ((aps[Is].reset()), ...);
            }

            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if<(I < numAPs), float>::type
                processAPsRecursive(float currentSignal)
            {
                // Get this AP's LFO index
                size_t idx = aps[I].getLfoIndex();
                // Safely read from the global LFO array
                float modValue = (globalLfoPtr != nullptr) ? globalLfoPtr[idx] : 0.0f;
                float next = aps[I].processSample(currentSignal, modValue);
                return processAPsRecursive<I + 1>(next);
            }

            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if<(I == numAPs), float>::type
                processAPsRecursive(float currentSignal)
            {
                return currentSignal;
            }
        };

    } // namespace multistage
} // namespace project
