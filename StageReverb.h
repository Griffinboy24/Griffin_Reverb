#pragma once
#include "ReverbCommon.h"
#include "ReverbSvf.h"  // New SVF filter header
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
                currentSampleRate = 44100.f;
            }

            void prepare(float sampleRate) {
                currentSampleRate = sampleRate;
                if constexpr (StageConfig::enableSVF) {
                    svfFilter.setParameters(StageConfig::svfCutoff, StageConfig::svfGain, sampleRate);
                }
                prepareAPs(sampleRate, std::make_index_sequence<numAPs>{});
            }

            void reset() {
                if constexpr (StageConfig::enableSVF) {
                    svfFilter.reset();
                }
                resetAPs(std::make_index_sequence<numAPs>{});
            }

            void setGlobalLfoOutputsPointer(const float* ptr) {
                globalLfoPtr = ptr;
            }

            JUCE_FORCEINLINE float processSample(float inSample)
            {
                float input = inSample;
                if constexpr (StageConfig::enableSVF) {
                    input = svfFilter.processSample(input);
                }
                return processAPsRecursive<0>(input);
            }

            void updateDelayTimes(float globalSize) {
                updateAPsDelayTimes(globalSize, std::make_index_sequence<numAPs>{});
            }

            void updateCoefficientScaling(float globalDensity) {
                updateAPsCoefficientScaling(globalDensity, std::make_index_sequence<numAPs>{});
            }

            // New: update SVF filter parameters if attached to user parameters.
            void updateSVFParameters(float cutoff, float dbGain) {
                if constexpr (StageConfig::enableSVF && StageConfig::attachSVF) {
                    svfFilter.setParameters(cutoff, dbGain, currentSampleRate);
                }
            }

        private:
            std::array<project::SimpleAP, numAPs> aps;
            const float* globalLfoPtr;
            // SVF filter is declared unconditionally but only used if enabled.
            LexiconShelvingFilter svfFilter;
            float currentSampleRate;

            template <size_t... Is>
            JUCE_FORCEINLINE void initAPs(std::index_sequence<Is...>)
            {
                ((aps[Is] = project::SimpleAP(
                    StageConfig::aps[Is].baseDelay,
                    StageConfig::aps[Is].coefficient,
                    StageConfig::aps[Is].lfoIndex,
                    StageConfig::scaleDelay,
                    StageConfig::scaleCoeff
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

            template <size_t... Is>
            JUCE_FORCEINLINE void updateAPsDelayTimes(float globalSize, std::index_sequence<Is...>)
            {
                ((aps[Is].updateDelayTime(globalSize)), ...);
            }

            template <size_t... Is>
            JUCE_FORCEINLINE void updateAPsCoefficientScaling(float globalDensity, std::index_sequence<Is...>)
            {
                ((aps[Is].updateCoefficientScaling(globalDensity)), ...);
            }
        };

    } // namespace multistage
} // namespace project
