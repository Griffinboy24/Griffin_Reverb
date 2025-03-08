#pragma once
#include "ReverbCommon.h"
#include <array>
#include <type_traits>

namespace project {
    namespace multistage {

        template <typename StageConfig>
        class StageReverb
        {
        public:
            static constexpr size_t numAPs = std::tuple_size<decltype(StageConfig::aps)>::value;

            StageReverb() {
                initAPs(std::make_index_sequence<numAPs>{});
                globalLfoPtr = nullptr;
            }

            void prepare(float sampleRate) {
                prepareAPs(sampleRate, std::make_index_sequence<numAPs>{});
            }

            void reset() {
                resetAPs(std::make_index_sequence<numAPs>{});
            }

            void setGlobalLfoOutputsPointer(const float* ptr) {
                globalLfoPtr = ptr;
            }

            JUCE_FORCEINLINE float processSample(float inSample)
            {
                return processAPsRecursive<0>(inSample);
            }

        private:
            std::array<project::SimpleAP, numAPs> aps;
            const float* globalLfoPtr;

            // Build allpasses from StageConfig::aps
            template <size_t... Is>
            JUCE_FORCEINLINE void initAPs(std::index_sequence<Is...>)
            {
                ((aps[Is] = project::SimpleAP(
                    StageConfig::aps[Is].baseDelay,
                    StageConfig::aps[Is].coefficient,
                    StageConfig::aps[Is].lfoIndex
                )), ...);
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
                processAPsRecursive(float current)
            {
                size_t idx = aps[I].getLfoIndex();
                float modVal = (globalLfoPtr != nullptr) ? globalLfoPtr[idx] : 0.f;
                float next = aps[I].processSample(current, modVal);
                return processAPsRecursive<I + 1>(next);
            }

            template <size_t I>
            JUCE_FORCEINLINE typename std::enable_if<(I == numAPs), float>::type
                processAPsRecursive(float current)
            {
                return current;
            }
        };

    } // namespace multistage
} // namespace project
