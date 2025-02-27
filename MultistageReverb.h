#pragma once
#include <array>
#include "MultistageReverbConfig.h"  // Defines project::multistage::StageConfig and MultistageReverbConfig
#include "StageReverb.h"             // Defines project::multistage::StageReverb

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // MultistageReverb Class
        //--------------------------------------------------------------------------
        // This class instantiates one StageReverb per stage defined in MultistageReverbConfig.
        // After each stage processes its input, the fixed, compile?time routing matrix is used
        // to generate a new input for each stage. The final output is produced by combining
        // (summing) the routed signals from all stages.
        class MultistageReverb {
        public:
            // Number of stages defined in the configuration.
            static constexpr size_t numStages = MultistageReverbConfig::stages.size();

            // Constructor: Default constructs one StageReverb per stage.
            MultistageReverb() = default;

            // prepare: Call prepare on each StageReverb with the given sample rate.
            void prepare(float sampleRate) {
                for (size_t i = 0; i < numStages; ++i) {
                    stages[i].prepare(sampleRate);
                }
            }

            // reset: Reset each StageReverb.
            void reset() {
                for (size_t i = 0; i < numStages; ++i) {
                    stages[i].reset();
                }
            }

            // processSample: Process a single input sample through each stage,
            // apply the send/receive routing, and return the final output.
            JUCE_FORCEINLINE float processSample(float input) {
                // Process each stage.
                std::array<float, numStages> stageOutputs{};
                for (size_t i = 0; i < numStages; ++i) {
                    stageOutputs[i] = stages[i].processSample(input);
                }

                // Compute routed inputs based on the routing matrix.
                std::array<float, numStages> routedInputs{};
                for (size_t dest = 0; dest < numStages; ++dest) {
                    float sum = 0.f;
                    for (size_t src = 0; src < numStages; ++src) {
                        sum += MultistageReverbConfig::routingMatrix[src][dest] * stageOutputs[src];
                    }
                    routedInputs[dest] = sum;
                }

                // Sum the routed inputs to produce the final output.
                float finalOutput = 0.f;
                for (size_t i = 0; i < numStages; ++i) {
                    finalOutput += routedInputs[i];
                }
                return finalOutput;
            }

        private:
            // Correct: Reference StageConfig directly (not as a member of MultistageReverbConfig).
            std::array<StageReverb<StageConfig>, numStages> stages;
        };

    } // namespace multistage
} // namespace project
