#pragma once
#include <array>
#include <tuple>
#include <type_traits>
#include "ReverbCommon.h"
#include "StageReverb.h"

namespace project {
    namespace multistage {

        template <typename Config>
        class MultiStageReverb
        {
        public:
            static constexpr size_t NumStages = Config::NumStages;
            static constexpr size_t NumNodes = Config::NumNodes;
            static constexpr size_t NumGlobalLFOs = Config::NumGlobalLFOs;

            static constexpr auto routingMatrix = Config::routingMatrix;

            // Global LFO array and output storage
            std::array<project::SimpleLFO, NumGlobalLFOs> globalLFOs;
            std::array<float, NumGlobalLFOs> globalLfoValues{};

            // Build a tuple of StageReverb objects for the stages.
            template <std::size_t I>
            using SingleStage = StageReverb<std::tuple_element_t<I, typename Config::StageTuple>>;

            template <std::size_t... Is>
            static constexpr auto buildStageTuple(std::index_sequence<Is...>)
            {
                return std::tuple<SingleStage<Is>...>{};
            }

            // The actual stages
            decltype(buildStageTuple(std::make_index_sequence<NumStages>{})) stages;

            // Node state array
            std::array<float, NumNodes> nodeState;

            MultiStageReverb()
            {
                nodeState.fill(0.f);
                // Initialize each LFO from config
                for (size_t i = 0; i < NumGlobalLFOs; ++i)
                {
                    float freq = Config::lfoFrequencies[i];
                    float amp = Config::lfoAmplitudes[i];
                    globalLFOs[i] = project::SimpleLFO(freq, amp);
                }
            }

            void prepare(float sampleRate)
            {
                // Prepare LFOs
                for (auto& l : globalLFOs) {
                    l.prepare(sampleRate);
                }
                // Prepare stages
                prepareStages(sampleRate, std::make_index_sequence<NumStages>{});
                setStageGlobalLfoPointers(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            void reset()
            {
                for (auto& lfo : globalLFOs) {
                    lfo.reset();
                }
                resetStages(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            JUCE_FORCEINLINE float processSample(float input)
            {
                // 1) Update LFO outputs
                for (size_t i = 0; i < NumGlobalLFOs; ++i) {
                    globalLfoValues[i] = globalLFOs[i].update();
                }

                // 2) Build new state array
                std::array<float, NumNodes> newState = nodeState;
                newState[0] = input;

                // 3) Process each stage
                processStagesCombined(newState, nodeState, std::make_index_sequence<NumStages>{});

                // 4) Compute final output from node (NumNodes-1)
                float out = computeDestination<NumNodes - 1>(newState);
                nodeState = newState;
                return out;
            }

            // New method to update delay times in all stages based on the global size parameter.
            void updateGlobalSizeParameter(float globalSize) {
                updateStagesDelayTimes(globalSize, std::make_index_sequence<NumStages>{});
            }

        private:
            template <size_t... Is>
            JUCE_FORCEINLINE void prepareStages(float sr, std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).prepare(sr)), ...);
            }

            template <size_t... Is>
            JUCE_FORCEINLINE void resetStages(std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).reset()), ...);
            }

            template <size_t... Is>
            JUCE_FORCEINLINE void setStageGlobalLfoPointers(std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).setGlobalLfoOutputsPointer(globalLfoValues.data())), ...);
            }

            template <size_t j>
            JUCE_FORCEINLINE float computeDestination(const std::array<float, NumNodes>& st)
            {
                return computeDestinationImpl<j>(st, std::make_index_sequence<NumNodes>{});
            }

            template <size_t j, size_t... Is>
            JUCE_FORCEINLINE float computeDestinationImpl(const std::array<float, NumNodes>& s,
                std::index_sequence<Is...>)
            {
                // Compute sum_{i=0}^{NumNodes-1} s[i]*routingMatrix[i][j]
                return ((s[Is] * routingMatrix[Is][j]) + ...);
            }

            // Unroll each stage: stage i -> node i+1
            template <size_t... Is>
            JUCE_FORCEINLINE void processStagesCombined(std::array<float, NumNodes>& newState,
                const std::array<float, NumNodes>& oldState,
                std::index_sequence<Is...>)
            {
                ((newState[1 + Is] = std::get<Is>(stages).processSample(
                    computeDestination<1 + Is>(oldState)
                )), ...);
            }

            // Helper to update delay times in all stages.
            template <size_t... Is>
            JUCE_FORCEINLINE void updateStagesDelayTimes(float globalSize, std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).updateDelayTimes(globalSize)), ...);
            }
        };

    } // namespace multistage
} // namespace project
