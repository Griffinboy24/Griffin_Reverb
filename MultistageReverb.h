#pragma once
#include <array>
#include <tuple>
#include <utility>
#include "MultistageReverbConfig.h"  // Our single config struct
#include "StageReverb.h"            // Templated stage
#include "ReverbCommon.h"           // SimpleAP, SimpleLFO

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb
        //
        //  - Creates a single global array of LFOs according to the config
        //  - Each stage uses these LFO outputs, read via a shared pointer
        //--------------------------------------------------------------------------
        class MultistageReverb {
        public:
            static constexpr size_t NumStages = MultistageReverbConfig::NumStages;
            static constexpr size_t NumNodes = MultistageReverbConfig::NumNodes;

            static_assert(MultistageReverbConfig::routingMatrix.size() == NumNodes,
                "routingMatrix must have NumNodes rows");
            static_assert(MultistageReverbConfig::routingMatrix[0].size() == NumNodes,
                "routingMatrix must have NumNodes columns");
            static_assert(MultistageReverbConfig::InputIndex == 0,
                "Input node must be index 0");
            static_assert(MultistageReverbConfig::OutputIndex == NumNodes - 1,
                "Output node must be index NumNodes-1");

            // Extract the stage types
            using StageTuple = MultistageReverbConfig::StageTuple;
            using StageConfig0 = std::tuple_element_t<0, StageTuple>;
            using StageConfig1 = std::tuple_element_t<1, StageTuple>;
            using StageConfig2 = std::tuple_element_t<2, StageTuple>;

            MultistageReverb()
            {
                nodeState.fill(0.f);

                // Build each global LFO from the config frequencies
                constexpr size_t count = MultistageReverbConfig::NumGlobalLFOs;
                for (size_t i = 0; i < count; ++i)
                {
                    float freq = MultistageReverbConfig::globalLfoFrequencies[i];
                    globalLFOs[i] = project::SimpleLFO(freq);
                }
            }

            void prepare(float sampleRate)
            {
                // Prepare global LFOs
                constexpr size_t count = MultistageReverbConfig::NumGlobalLFOs;
                for (size_t i = 0; i < count; ++i)
                    globalLFOs[i].prepare(sampleRate);

                // Prepare each stage, then set them to read from globalLfoValues
                prepareStages(sampleRate, std::make_index_sequence<NumStages>{});
                setStageGlobalLfoPointers(std::make_index_sequence<NumStages>{});

                nodeState.fill(0.f);
            }

            void reset()
            {
                resetStages(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            // Process one sample:
            //  1) Update all LFO outputs
            //  2) Route signals among stages
            JUCE_FORCEINLINE float processSample(float input)
            {
                // Update global LFO array
                constexpr size_t count = MultistageReverbConfig::NumGlobalLFOs;
                for (size_t i = 0; i < count; ++i)
                    globalLfoValues[i] = globalLFOs[i].update();

                // Prepare the next node state
                std::array<float, NumNodes> newState = nodeState;
                newState[MultistageReverbConfig::InputIndex] = input;

                processStagesCombined(newState, nodeState, std::make_index_sequence<NumStages>{});
                float out = computeDestination<MultistageReverbConfig::OutputIndex>(newState);

                // Commit
                nodeState = newState;
                return out;
            }

        private:
            // Our actual stages
            std::tuple<
                StageReverb<StageConfig0>,
                StageReverb<StageConfig1>,
                StageReverb<StageConfig2>
            > stages;

            // Node state for the reverb network
            std::array<float, NumNodes> nodeState;

            //==================== Global LFOs + outputs ====================//
            std::array<project::SimpleLFO, MultistageReverbConfig::NumGlobalLFOs> globalLFOs;
            std::array<float, MultistageReverbConfig::NumGlobalLFOs> globalLfoValues{};

            //==================== Prepare & Reset ====================//
            template <std::size_t... Is>
            JUCE_FORCEINLINE void prepareStages(float sr, std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).prepare(sr)), ...);
            }
            template <std::size_t... Is>
            JUCE_FORCEINLINE void resetStages(std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).reset()), ...);
            }
            template <std::size_t... Is>
            JUCE_FORCEINLINE void setStageGlobalLfoPointers(std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).setGlobalLfoOutputsPointer(globalLfoValues.data())), ...);
            }

            //==================== Routing Helpers ====================//
            template <size_t j>
            JUCE_FORCEINLINE float computeDestination(const std::array<float, NumNodes>& st)
            {
                return computeDestinationImpl<j>(st, std::make_index_sequence<NumNodes>{});
            }
            template <size_t j, size_t... Is>
            JUCE_FORCEINLINE float computeDestinationImpl(const std::array<float, NumNodes>& st,
                std::index_sequence<Is...>)
            {
                // sum_{i=0}^{NumNodes-1} [ st[i] * routingMatrix[i][j] ]
                return ((st[Is] * MultistageReverbConfig::routingMatrix[Is][j]) + ...);
            }

            template <std::size_t... Is>
            JUCE_FORCEINLINE void processStagesCombined(std::array<float, NumNodes>& newState,
                const std::array<float, NumNodes>& oldState,
                std::index_sequence<Is...>)
            {
                // For each stage i:
                //   newState[FirstStageIndex + i] =
                //       stage[i].processSample( routing from oldState )
                ((newState[MultistageReverbConfig::FirstStageIndex + Is] =
                    std::get<Is>(stages).processSample(
                        computeDestination<MultistageReverbConfig::FirstStageIndex + Is>(oldState)
                    )
                    ), ...);
            }
        };

    } // namespace multistage
} // namespace project
