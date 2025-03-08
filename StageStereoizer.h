#pragma once
#include "ReverbCommon.h"

namespace project {
    namespace multistage {

        class StageStereoizer {
        public:
            // Construct left/right allpasses with both delay scaling and coefficient scaling enabled.
            StageStereoizer()
                : leftAP(2200.f, 0.5f, 0, true, true),  // base=2200, coeff=0.5, lfoIndex=0, scaleDelay=true, scaleCoefficient=true
                rightAP(2000.f, 0.5f, 1, true, true),   // base=2000, coeff=0.5, lfoIndex=1, scaleDelay=true, scaleCoefficient=true
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

            // Update delay times for both channels based on the global size parameter.
            void updateDelayTimes(float globalSize) {
                leftAP.updateDelayTime(globalSize);
                rightAP.updateDelayTime(globalSize);
            }

            // Update coefficient scaling (density) for both channels based on the global density parameter.
            void updateCoefficientScaling(float globalDensity) {
                leftAP.updateCoefficientScaling(globalDensity);
                rightAP.updateCoefficientScaling(globalDensity);
            }

        private:
            project::SimpleAP leftAP;
            project::SimpleAP rightAP;
            const float* globalLfoPtr;
        };

    } // namespace multistage
} // namespace project
