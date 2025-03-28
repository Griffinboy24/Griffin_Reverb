#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>

#ifndef JUCE_FORCEINLINE
#if defined(_MSC_VER)
#define JUCE_FORCEINLINE __forceinline
#else
#define JUCE_FORCEINLINE inline __attribute__((always_inline))
#endif
#endif

// Build a std::array at compile time from variadic arguments
template <typename T, typename... Ts>
constexpr std::array<typename std::common_type<T, Ts...>::type, 1 + sizeof...(Ts)>
ms_make_array(T t, Ts... ts)
{
    return { t, ts... };
}

namespace project {

    //==============================================================
    // SimpleLFO: now has amplitude for global depth
    //==============================================================
    class SimpleLFO {
    public:
        SimpleLFO() : frequency(1.f), amplitude(1.f), phase(0.f), sampleRate(44100.f), increment(0.f) {}
        SimpleLFO(float freq, float amp)
            : frequency(freq), amplitude(amp), phase(0.f), sampleRate(44100.f), increment(0.f)
        {
        }

        void prepare(float sr) {
            sampleRate = sr;
            phase = 0.f;
            increment = frequency / sampleRate;
        }

        void reset() {
            phase = 0.f;
        }

        // Returns amplitude * approximate sine
        JUCE_FORCEINLINE float update() {
            phase += increment;
            if (phase >= 1.f) {
                phase -= 1.f;
            }
            return amplitude * parSin(phase);
        }

        // Set amplitude or frequency at runtime
        void setAmplitude(float newAmp) { amplitude = newAmp; }
        void setFrequency(float newFreq) {
            frequency = newFreq;
            increment = frequency / sampleRate;
        }

    private:
        float frequency;
        float amplitude;
        float phase;
        float sampleRate;
        float increment;

        JUCE_FORCEINLINE float parSin(float ph) const {
            float shifted = 0.5f - ph;
            return shifted * (8.f - 16.f * std::fabs(shifted));
        }
    };

    //==============================================================
    // SimpleAP: uses baseDelay + lfoValue and supports precomputed scaling.
    // Now allocates the maximum delay buffer size from the start,
    // assuming the global size will only ever go up to 2�.
    //==============================================================
    class SimpleAP {
    public:
        SimpleAP()
            : originalBaseDelay(0.f), effectiveBaseDelay(0.f),
            originalCoefficient(0.f), effectiveCoefficient(0.f),
            lfoIndex(0), sampleRate(44100.f), writeIndex(0),
            powerBufferSize(0), indexMask(0), scaleDelay(false), scaleCoefficient(false)
        {
        }

        // Constructor with scale flags for delay and coefficient (density)
        SimpleAP(float baseD, float coeff, size_t lfoIdx, bool scaleDelayFlag, bool scaleCoeffFlag)
            : originalBaseDelay(baseD), effectiveBaseDelay(baseD),
            originalCoefficient(coeff), effectiveCoefficient(coeff),
            lfoIndex(lfoIdx), sampleRate(44100.f), writeIndex(0),
            scaleDelay(scaleDelayFlag), scaleCoefficient(scaleCoeffFlag)
        {
            maxDelay = effectiveBaseDelay + 50.f;
        }

        void prepare(float sr) {
            sampleRate = sr;
            writeIndex = 0;
            // Allocate maximum needed delay buffer size, assuming globalSize can be up to 2�.
            float maxExpectedDelay = originalBaseDelay * 2.f + 50.f;
            int reqSize = static_cast<int>(std::ceil(maxExpectedDelay)) + 4;
            powerBufferSize = nextPow2(reqSize);
            indexMask = powerBufferSize - 1;
            delayBuffer.assign(powerBufferSize, 0.f);
        }

        void reset() {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.f);
            writeIndex = 0;
        }

        // Update effective delay based on the global size parameter if scaling is enabled.
        // Since the delay buffer was allocated for the maximum delay, no reallocation is needed.
        void updateDelayTime(float globalSize) {
            if (scaleDelay) {
                effectiveBaseDelay = originalBaseDelay * globalSize;
                maxDelay = effectiveBaseDelay + 50.f;
            }
        }

        // Update effective coefficient based on the global density parameter if scaling is enabled.
        void updateCoefficientScaling(float globalDensity) {
            if (scaleCoefficient) {
                effectiveCoefficient = originalCoefficient * globalDensity;
            }
        }

        JUCE_FORCEINLINE float processSample(float x, float lfoValue) {
            float targetDelay = effectiveBaseDelay + lfoValue;
            if (targetDelay < 0.f) {
                targetDelay = 0.f;
            }

            int d_int = static_cast<int>(targetDelay);
            float d_frac = targetDelay - static_cast<float>(d_int);

            bool hasFraction = (d_frac > 0.f);
            int offset = hasFraction ? 1 : 0;
            int index0 = (writeIndex - d_int - offset) & indexMask;
            float frac = hasFraction ? (1.f - d_frac) : 0.f;
            int index1 = (index0 + 1) & indexMask;

            float delayedV = (1.f - frac) * delayBuffer[index0]
                + (frac)*delayBuffer[index1];

                float v = x - effectiveCoefficient * delayedV;
                float y = effectiveCoefficient * v + delayedV;

                delayBuffer[writeIndex] = v;
                writeIndex = (writeIndex + 1) & indexMask;
                return y;
        }

        size_t getLfoIndex() const { return lfoIndex; }

    private:
        static int nextPow2(int x) {
            x--;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            return x + 1;
        }

        float originalBaseDelay;
        float effectiveBaseDelay;
        float maxDelay;
        float originalCoefficient;
        float effectiveCoefficient;
        size_t lfoIndex;
        float sampleRate;
        std::vector<float> delayBuffer;
        int writeIndex;
        int powerBufferSize;
        int indexMask;
        bool scaleDelay;
        bool scaleCoefficient;
    };

} // namespace project
