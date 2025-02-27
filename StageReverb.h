#pragma once
#include <array>
#include <tuple>
#include "ReverbCommon.h"  // Contains ms_make_array, SimpleLFO, and SimpleAP

namespace project {
    namespace multistage {

        //--------------------------------------------------------------------------
        // StageReverb Class Template
        //--------------------------------------------------------------------------
        // This class template processes a single stage based on a StageConfig.
        // It instantiates fixed-size arrays of SimpleLFO and SimpleAP objects using the
        // configuration values from StageConfig::lfoFrequencies and StageConfig::aps.
        template <typename StageConfig>
        class StageReverb {
        public:
            // Determine the number of LFOs and APs from the configuration.
            static constexpr size_t numLFOs = std::tuple_size<decltype(StageConfig::lfoFrequencies)>::value;
            static constexpr size_t numAPs = std::tuple_size<decltype(StageConfig::aps)>::value;

            // Constructor: Initialize LFOs and APs using the configuration.
            StageReverb() {
                for (size_t i = 0; i < numLFOs; ++i) {
                    lfos[i] = SimpleLFO(StageConfig::lfoFrequencies[i]);
                }
                for (size_t i = 0; i < numAPs; ++i) {
                    const auto& apCfg = StageConfig::aps[i];
                    aps[i] = SimpleAP(apCfg.baseDelay, apCfg.coefficient, apCfg.depth, apCfg.lfoIndex);
                }
            }

            // prepare(sampleRate): Call prepare on each LFO and each AP.
            void prepare(float sampleRate) {
                for (size_t i = 0; i < numLFOs; ++i)
                    lfos[i].prepare(sampleRate);
                for (size_t i = 0; i < numAPs; ++i)
                    aps[i].prepare(sampleRate);
            }

            // reset(): Reset each AP.
            void reset() {
                for (size_t i = 0; i < numAPs; ++i)
                    aps[i].reset();
            }

            // processSample(input):
            //   - Updates LFOs to get modulation values.
            //   - Processes the input sample sequentially through each AP (using the configured LFO index).
            //   - Returns the processed output.
            JUCE_FORCEINLINE float processSample(float input) {
                std::array<float, numLFOs> lfoValues;
                for (size_t i = 0; i < numLFOs; ++i) {
                    lfoValues[i] = lfos[i].update();
                }
                float output = input;
                for (size_t i = 0; i < numAPs; ++i) {
                    size_t idx = aps[i].getLfoIndex();
                    if (idx >= numLFOs)
                        idx = 0;
                    output = aps[i].processSample(output, lfoValues[idx]);
                }
                return output;
            }

        private:
            // Fixed-size arrays of LFOs and APs.
            std::array<SimpleLFO, numLFOs> lfos;
            std::array<SimpleAP, numAPs> aps;
        };

    } // namespace multistage
} // namespace project
