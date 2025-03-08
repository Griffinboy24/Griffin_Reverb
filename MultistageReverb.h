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
            static constexpr size_t NumNodes = Config::NumNodes; // (NumStages + 2)
            static constexpr size_t NumGlobalLFOs = Config::NumGlobalLFOs;
            static constexpr size_t NumConnections = Config::connections.size();

            // Global LFO array and output storage.
            std::array<project::SimpleLFO, NumGlobalLFOs> globalLFOs;
            std::array<float, NumGlobalLFOs> globalLfoValues{};

            // Build a tuple of StageReverb objects.
            template <std::size_t I>
            using SingleStage = StageReverb<std::tuple_element_t<I, typename Config::StageTuple>>;

            template <std::size_t... Is>
            static constexpr auto buildStageTuple(std::index_sequence<Is...>)
            {
                return std::tuple<SingleStage<Is>...>{};
            }

            decltype(buildStageTuple(std::make_index_sequence<NumStages>{})) stages;

            // Node state array.
            std::array<float, NumNodes> nodeState;

            // Effective connection weights (updated on feedback parameter changes).
            std::array<float, NumConnections> effectiveWeights;

            MultiStageReverb()
            {
                nodeState.fill(0.f);
                // Initialize LFOs.
                for (size_t i = 0; i < NumGlobalLFOs; ++i)
                {
                    float freq = Config::lfoFrequencies[i];
                    float amp = Config::lfoAmplitudes[i];
                    globalLFOs[i] = project::SimpleLFO(freq, amp);
                }
                // Initialize effective weights using default feedback parameter = 1.0.
                for (size_t i = 0; i < NumConnections; ++i)
                {
                    effectiveWeights[i] = Config::connections[i].baseWeight;
                }
            }

            void prepare(float sampleRate)
            {
                // Prepare LFOs.
                for (auto& l : globalLFOs) {
                    l.prepare(sampleRate);
                }
                // Prepare stages.
                prepareStages(sampleRate, std::make_index_sequence<NumStages>{});
                setStageGlobalLfoPointers(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            void reset()
            {
                for (auto& l : globalLFOs) {
                    l.reset();
                }
                resetStages(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            // Process one sample.
            JUCE_FORCEINLINE float processSample(float input)
            {
                // 1) Update LFO outputs.
                for (size_t i = 0; i < NumGlobalLFOs; ++i)
                {
                    globalLfoValues[i] = globalLFOs[i].update();
                }
                // 2) Copy current state and set input.
                std::array<float, NumNodes> newState = nodeState;
                newState[0] = input;

                // 3) Process each stage (nodes 1..NumStages) via a compile-time unrolled loop.
                processStages(newState, nodeState, std::make_index_sequence<NumStages>{});

                // 4) Compute final output (node NumNodes-1) from new state.
                {
                    float sum = 0.f;
                    for (size_t i = 0; i < NumConnections; ++i)
                    {
                        if (Config::connections[i].dst == NumNodes - 1)
                        {
                            size_t src = Config::connections[i].src;
                            sum += newState[src] * effectiveWeights[i];
                        }
                    }
                    newState[NumNodes - 1] = sum;
                }

                // 5) Update node state and return final output.
                nodeState = newState;
                return newState[NumNodes - 1];
            }

            // Update feedback parameter: for connections flagged with scaleFeedback,
            // effectiveWeight = baseWeight * feedbackParam; others remain unchanged.
            void updateFeedbackParameter(float feedbackParam)
            {
                for (size_t i = 0; i < NumConnections; ++i)
                {
                    if (Config::connections[i].scaleFeedback)
                        effectiveWeights[i] = Config::connections[i].baseWeight * feedbackParam;
                    else
                        effectiveWeights[i] = Config::connections[i].baseWeight;
                }
            }

            // Update global size parameter for delays (passes to stages).
            void updateGlobalSizeParameter(float globalSize)
            {
                updateStagesDelayTimes(globalSize, std::make_index_sequence<NumStages>{});
            }

        private:
            // Helper: process a single stage.
            template <size_t I>
            JUCE_FORCEINLINE void processStage(std::array<float, NumNodes>& newState,
                const std::array<float, NumNodes>& oldState)
            {
                constexpr size_t dest = I + 1;
                float sum = 0.f;
                for (size_t j = 0; j < NumConnections; ++j)
                {
                    if (Config::connections[j].dst == dest)
                    {
                        size_t src = Config::connections[j].src;
                        sum += oldState[src] * effectiveWeights[j];
                    }
                }
                newState[dest] = std::get<I>(stages).processSample(sum);
            }

            // Helper: unroll processing of all stages.
            template <size_t... Is>
            JUCE_FORCEINLINE void processStages(std::array<float, NumNodes>& newState,
                const std::array<float, NumNodes>& oldState,
                std::index_sequence<Is...>)
            {
                (processStage<Is>(newState, oldState), ...);
            }

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

            template <size_t... Is>
            JUCE_FORCEINLINE void updateStagesDelayTimes(float globalSize, std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).updateDelayTimes(globalSize)), ...);
            }
        };

    } // namespace multistage
} // namespace project
 