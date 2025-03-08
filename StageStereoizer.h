#pragma once
#include "ReverbCommon.h"

namespace project {
namespace multistage {

    class StageStereoizer {
    public:
        // Suppose we want 2 allpasses, each referencing a global LFO by index
        // We pick indices arbitrarily, or define them in some config if needed.
        StageStereoizer()
            : leftAP(2200.f, 0.5f, 0),  // base=2200, coeff=0.5, lfoIndex=0
              rightAP(2000.f, 0.5f, 1), // base=2000, coeff=0.5, lfoIndex=1
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

        // The final code also sets where we read from the global LFOs
        void setGlobalLfoOutputsPointer(const float* ptr) {
            globalLfoPtr = ptr;
        }

        inline void processSample(float monoIn, float& leftOut, float& rightOut) {
            float lVal = 0.f;
            float rVal = 0.f;
            if (globalLfoPtr) {
                // read the indexes from each AP
                size_t lIdx = leftAP.getLfoIndex();
                size_t rIdx = rightAP.getLfoIndex();
                lVal = globalLfoPtr[lIdx];
                rVal = globalLfoPtr[rIdx];
            }
            leftOut  = leftAP.processSample(monoIn, lVal);
            rightOut = rightAP.processSample(monoIn, rVal);
        }

    private:
        project::SimpleAP leftAP;
        project::SimpleAP rightAP;
        const float* globalLfoPtr;
    };

} // namespace multistage
} // namespace project
