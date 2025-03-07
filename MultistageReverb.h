#pragma once
#include <array>
#include <tuple>
#include <utility>
#include "MultistageReverbConfig.h"  // Contains configuration constants
#include "StageReverb.h"            // Templated on the configuration type
#include "ReverbCommon.h"           // Provides SimpleLFO, etc.

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb
        //
        //  - Now has a single global array of LFOs (4 total), plus an array
        //    that holds their updated values each sample.
        //  - Each stage is assigned a pointer to this global-LFO array so that
        //    all allpass filters read from the same set of LFOs, avoiding duplication.
        //--------------------------------------------------------------------------
        class MultistageReverb {
        public:
            static constexpr size_t NumStages = MultistageReverbConfig::NumStages;
            static constexpr size_t NumNodes = MultistageReverbConfig::NumNodes;

            static_assert(MultistageReverbConfig::routingMatrix.size() == NumNodes,
                "Routing matrix must have NumNodes rows");
            static_assert(MultistageReverbConfig::routingMatrix[0].size() == NumNodes,
                "Routing matrix must have NumNodes columns");
            static_assert(MultistageReverbConfig::InputIndex == 0,
                "Input node must be index 0");
            static_assert(MultistageReverbConfig::OutputIndex == NumNodes - 1,
                "Output node must be index NumNodes-1");

            // We unify the reverb to have exactly 4 global LFO frequencies:
            //    0 => 0.1
            //    1 => 0.9128
            //    2 => 1.0
            //    3 => 1.1341
            static constexpr size_t NumGlobalLFOs = 3;

            MultistageReverb()
            {
                nodeState.fill(0.f);

                // Define global LFO frequencies
                globalLFOs[0] = project::SimpleLFO(0.1f);
                globalLFOs[1] = project::SimpleLFO(0.9128f);
                globalLFOs[2] = project::SimpleLFO(1.1341f);
            }

            void prepare(float sampleRate)
            {
                // Prepare global LFOs
                for (auto& lfo : globalLFOs)
                    lfo.prepare(sampleRate);

                // Prepare each stage
                prepareStages(sampleRate, std::make_index_sequence<NumStages>{});

                // Make each stage read from globalLfoValues
                setStageGlobalLfoPointers(std::make_index_sequence<NumStages>{});

                nodeState.fill(0.f);
            }

            void reset()
            {
                // (Optional) If you want to clear LFO phases, we can simply re-prepare them;
                // but SimpleLFO also resets phase on prepare. No dedicated reset needed unless
                // you want to flush them to a known phase.
                resetStages(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            // processSample: Updates the global LFO values once, then processes the node network.
            JUCE_FORCEINLINE float processSample(float input)
            {
                // Update all global LFO outputs just once per sample
                for (size_t i = 0; i < NumGlobalLFOs; ++i)
                    globalLfoValues[i] = globalLFOs[i].update();

                // The rest is the same: we propagate the new node state forward
                std::array<float, NumNodes> newState = nodeState;
                newState[MultistageReverbConfig::InputIndex] = input;

                processStagesCombined(newState, nodeState, std::make_index_sequence<NumStages>{});

                float out = computeDestination<MultistageReverbConfig::OutputIndex>(newState);
                nodeState = newState;
                return out;
            }

        private:
            using StageConfig0 = std::tuple_element_t<0, MultistageReverbConfig::StageTuple>;
            using StageConfig1 = std::tuple_element_t<1, MultistageReverbConfig::StageTuple>;
            using StageConfig2 = std::tuple_element_t<2, MultistageReverbConfig::StageTuple>;

            // Tuple of stages
            std::tuple<StageReverb<StageConfig0>,
                StageReverb<StageConfig1>,
                StageReverb<StageConfig2>> stages;

            // Node state for the reverb network
            std::array<float, NumNodes> nodeState;

            //==================== Global LFOs + their outputs ====================

            // We keep 4 LFOs total, used by all stages
            std::array<project::SimpleLFO, NumGlobalLFOs> globalLFOs;

            // We store the updated output of each LFO for the current sample
            std::array<float, NumGlobalLFOs> globalLfoValues{};

            // Assign that pointer to each stage so each AP can do: globalLfoPtr[lfoIndex]
            template <std::size_t... Is>
            JUCE_FORCEINLINE void setStageGlobalLfoPointers(std::index_sequence<Is...>)
            {
                ((std::get<Is>(stages).setGlobalLfoOutputsPointer(globalLfoValues.data())), ...);
            }

            //==================== Stage Prep & Reset ====================
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

            //==================== Routing computation ====================
            template <size_t j>
            JUCE_FORCEINLINE float computeDestination(const std::array<float, NumNodes>& state)
            {
                return computeDestinationImpl<j>(state, std::make_index_sequence<NumNodes>{});
            }
            template <size_t j, size_t... Is>
            JUCE_FORCEINLINE float computeDestinationImpl(const std::array<float, NumNodes>& state,
                std::index_sequence<Is...>)
            {
                // sum_{i=0}^{NumNodes-1} ( state[i] * routingMatrix[i][j] )
                return ((state[Is] * MultistageReverbConfig::routingMatrix[Is][j]) + ...);
            }

            //==================== Combine stage outputs into nodeState ====================
            template <std::size_t... Is>
            JUCE_FORCEINLINE void processStagesCombined(std::array<float, NumNodes>& newState,
                const std::array<float, NumNodes>& oldState,
                std::index_sequence<Is...>)
            {
                // For stage i, we read from oldState, processSample, store result in newState
                ((newState[MultistageReverbConfig::FirstStageIndex + Is] =
                    std::get<Is>(stages).processSample(
                        computeDestination<MultistageReverbConfig::FirstStageIndex + Is>(oldState)
                    )
                    ), ...);
            }
        };

    } // namespace multistage
} // namespace project
