#pragma once
#include <array>
#include <tuple>
#include <utility>
#include "MultistageReverbConfig.h"  // Contains configuration constants.
#include "StageReverb.h"             // Templated on the configuration type.
#include "ReverbCommon.h"            // Provides common DSP utilities.

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb Class
        //
        // Persistent Node Network:
        //   - A persistent nodeState array is maintained across samples.
        //   - Each new sample is processed using the previous sample's state,
        //     naturally introducing a one-sample delay in the feedback paths.
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

            MultistageReverb() {
                nodeState.fill(0.f);
            }

            void prepare(float sampleRate) {
                prepareStages(sampleRate, std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }
            void reset() {
                resetStages(std::make_index_sequence<NumStages>{});
                nodeState.fill(0.f);
            }

            // processSample: Processes one sample using the persistent node network.
            // The new node state is computed from the previous state, which allows
            // the feedback from later stages to be incorporated (with a one-sample delay).
            JUCE_FORCEINLINE float processSample(float input) {
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
            std::tuple< StageReverb<StageConfig0>, StageReverb<StageConfig1>, StageReverb<StageConfig2> > stages;
            std::array<float, NumNodes> nodeState;

            // Prepare stage processors.
            template <std::size_t... Is>
            JUCE_FORCEINLINE void prepareStages(float sr, std::index_sequence<Is...>) {
                ((std::get<Is>(stages).prepare(sr)), ...);
            }
            template <std::size_t... Is>
            JUCE_FORCEINLINE void resetStages(std::index_sequence<Is...>) {
                ((std::get<Is>(stages).reset()), ...);
            }

            // Routing computation: for destination node j, compute:
            //   sum_{i=0}^{NumNodes-1} ( state[i] * routingMatrix[i][j] )
            template <size_t j>
            JUCE_FORCEINLINE float computeDestination(const std::array<float, NumNodes>& state) {
                return computeDestinationImpl<j>(state, std::make_index_sequence<NumNodes>{});
            }
            template <size_t j, size_t... Is>
            JUCE_FORCEINLINE float computeDestinationImpl(const std::array<float, NumNodes>& state,
                std::index_sequence<Is...>) {
                return ((state[Is] * MultistageReverbConfig::routingMatrix[Is][j]) + ...);
            }

            // Process each stage: update newState for stage i using the previous state.
            template <std::size_t... Is>
            JUCE_FORCEINLINE void processStagesCombined(std::array<float, NumNodes>& newState,
                const std::array<float, NumNodes>& oldState,
                std::index_sequence<Is...>) {
                ((newState[MultistageReverbConfig::FirstStageIndex + Is] =
                    std::get<Is>(stages).processSample(
                        computeDestination<MultistageReverbConfig::FirstStageIndex + Is>(oldState)
                    )
                    ), ...);
            }
        };

    } // namespace multistage
} // namespace project
