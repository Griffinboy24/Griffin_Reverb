#pragma once
#include <array>
#include <utility>
#include "MultistageReverbConfig.h"  // Contains the (N+2)x(N+2) routing matrix and symbolic indices.
#include "StageReverb.h"             // Defines the per-stage processing (LFO & AP chain).

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb Class
        //
        // New Routing Convention:
        //   - Rows represent the source nodes.
        //   - Columns represent the destination nodes.
        // For each destination node j, its value is computed
        //
        // Integrated Stage Processing:
        //   - For each stage node (indices FirstStageIndex to FirstStageIndex+numStages-1),
        //     we compute the routed input for that node and immediately pass it through
        //     its processor.
        //   - Finally, the output node is computed from the updated node array.
        // This single-pass integration prevents double-summing that can cause feedback.
        //--------------------------------------------------------------------------
        class MultistageReverb {
        public:
            // Number of reverb stages.
            static constexpr size_t numStages = MultistageReverbConfig::stages.size();
            // Total nodes = input + stages + output.
            static constexpr size_t NumNodes = MultistageReverbConfig::NumNodes;

            // --- Static Validation ---
            static_assert(MultistageReverbConfig::routingMatrix.size() == NumNodes,
                "Routing matrix must have NumNodes rows");
            static_assert(MultistageReverbConfig::routingMatrix[0].size() == NumNodes,
                "Routing matrix must have NumNodes columns");
            static_assert(MultistageReverbConfig::InputIndex == 0,
                "Input node must be index 0");
            static_assert(MultistageReverbConfig::OutputIndex == NumNodes - 1,
                "Output node must be index NumNodes-1");

            MultistageReverb() = default;

            void prepare(float sampleRate) {
                prepareStages(sampleRate, std::make_index_sequence<numStages>{});
            }
            void reset() {
                resetStages(std::make_index_sequence<numStages>{});
            }

            // processSample: Processes one sample through the node network.
            //   - The node array is initialized with the input at node 0.
            //   - For each stage node, its routed input is computed and processed immediately.
            //   - The final output is computed from the updated node array.
            JUCE_FORCEINLINE float processSample(float input) {
                std::array<float, NumNodes> nodeArray{}; // Initialize all nodes to 0.
                nodeArray[MultistageReverbConfig::InputIndex] = input; // Place the external input.

                // --- Integrated Stage Processing ---
                // For each stage, compute its routed input (from all nodes) and process it.
                processStagesCombined(nodeArray, std::make_index_sequence<numStages>{});

                // --- Final Routing ---
                // Compute the output node's value from the updated node array.
                return computeDestination<MultistageReverbConfig::OutputIndex>(nodeArray);
            }

        private:
            // Array of stage processors; stage i corresponds to node index (FirstStageIndex + i).
            std::array<StageReverb<StageConfig>, numStages> stages;

            // --- Stage Preparation and Reset Helpers ---
            template <size_t... Is>
            JUCE_FORCEINLINE void prepareStages(float sr, std::index_sequence<Is...>) {
                ((stages[Is].prepare(sr)), ...);
            }
            template <size_t... Is>
            JUCE_FORCEINLINE void resetStages(std::index_sequence<Is...>) {
                ((stages[Is].reset()), ...);
            }

            // --- Routing Computation ---
            // Computes the value for a destination node j 
            template <size_t j>
            JUCE_FORCEINLINE float computeDestination(const std::array<float, NumNodes>& nodeArray) {
                return computeDestinationImpl<j>(nodeArray, std::make_index_sequence<NumNodes>{});
            }
            template <size_t j, size_t... Is>
            JUCE_FORCEINLINE float computeDestinationImpl(const std::array<float, NumNodes>& nodeArray,
                std::index_sequence<Is...>) {
                return ((nodeArray[Is] * MultistageReverbConfig::routingMatrix[Is][j]) + ...);
            }

            // --- Integrated Stage Processing Helper ---
            // For each stage (indexed by i), do:
            //   - Compute its routed input using the routing matrix at destination (FirstStageIndex + i).
            //   - Process that routed input with the stage processor.
            //   - Store the processed output back into nodeArray at index (FirstStageIndex + i).
            template <size_t... Is>
            JUCE_FORCEINLINE void processStagesCombined(std::array<float, NumNodes>& nodeArray,
                std::index_sequence<Is...>) {
                ((nodeArray[MultistageReverbConfig::FirstStageIndex + Is] =
                    stages[Is].processSample(
                        computeDestination<MultistageReverbConfig::FirstStageIndex + Is>(nodeArray)
                    )), ...);
            }
        };

    } // namespace multistage
} // namespace project
