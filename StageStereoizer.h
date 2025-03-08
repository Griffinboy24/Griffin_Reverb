#pragma once
#include "ReverbCommon.h"

namespace project {
    namespace multistage {

        class StageStereoizer {
        public:
            // Construct left/right allpasses with the scale flag enabled.
            StageStereoizer()
                : leftAP(2200.f, 0.5f, 0, true),  // base=2200, coeff=0.5, lfoIndex=0, scalable=true
                rightAP(2000.f, 0.5f, 1, true), // base=2000, coeff=0.5, lfoIndex=1, scalable=true
                globalLfoPtr(nullptr)
            {
            }

            void prepare(float sampleRate) {
                leftAP.prepare(sampleRate);
                rightAP.prepare(sampleRate);
            }

            void reset() {
                leftAP.reset();
                rightAP.reset();
            }

            // Sets the pointer to the global LFO outputs.
            void setGlobalLfoOutputsPointer(const float* ptr) {
                globalLfoPtr = ptr;
            }

            // Processes a sample to produce stereo output.
            inline void processSample(float monoIn, float& leftOut, float& rightOut) {
                float lVal = 0.f;
                float rVal = 0.f;
                if (globalLfoPtr) {
                    size_t lIdx = leftAP.getLfoIndex();
                    size_t rIdx = rightAP.getLfoIndex();
                    lVal = globalLfoPtr[lIdx];
                    rVal = globalLfoPtr[rIdx];
                }
                leftOut = leftAP.processSample(monoIn, lVal);
                rightOut = rightAP.processSample(monoIn, rVal);
            }

            // New: Update delay times for both channels based on the global size parameter.
            void updateDelayTimes(float globalSize) {
                leftAP.updateDelayTime(globalSize);
                rightAP.updateDelayTime(globalSize);
            }

        private:
            project::SimpleAP leftAP;
            project::SimpleAP rightAP;
            const float* globalLfoPtr;
        };

    } // namespace multistage
} // namespace project
