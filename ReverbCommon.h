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

template <typename T, typename... Ts>
constexpr std::array<typename std::common_type<T, Ts...>::type, 1 + sizeof...(Ts)>
ms_make_array(T t, Ts... ts) {
    return { t, ts... };
}

namespace project {

    //==============================================================================
    // Simple LFO Class (unchanged)
    //==============================================================================
    class SimpleLFO {
    public:
        SimpleLFO() : frequency(1.f), phase(0.f), sampleRate(44100.f), increment(0.f) {}
        SimpleLFO(float freq) : frequency(freq), phase(0.f), sampleRate(44100.f), increment(0.f) {}

        void prepare(float sr) {
            sampleRate = sr;
            phase = 0.f;
            increment = frequency / sampleRate;
        }
        JUCE_FORCEINLINE float update() {
            phase += increment;
            if (phase >= 1.f)
                phase -= 1.f;
            return parSin(phase);
        }
    private:
        float frequency;
        float phase;
        float sampleRate;
        float increment;
        JUCE_FORCEINLINE float parSin(float ph) const {
            float shifted = 0.5f - ph;
            return shifted * (8.f - 16.f * std::fabs(shifted));
        }
    };

    //==============================================================================
    // Simple Allpass Delay Line Class (SimpleAP)
    //
    // The modulated delay time is computed as: targetDelay = baseDelay + depth * lfoValue.
    //==============================================================================
    class SimpleAP {
    public:
        SimpleAP()
            : baseDelay(0.f), maxDelay(0.f), coefficient(0.f), depth(0.f), lfoIndex(0),
            sampleRate(44100.f), writeIndex(0),
            powerBufferSize(0), indexMask(0)
        {
        }
        SimpleAP(float baseDelay, float coeff, float d, size_t lfoIdx)
            : baseDelay(baseDelay), coefficient(coeff), depth(d), lfoIndex(lfoIdx),
            sampleRate(44100.f), writeIndex(0)
        {
            maxDelay = baseDelay + 50.f;
        }

        void prepare(float sr) {
            sampleRate = sr;
            writeIndex = 0;
            int reqSize = static_cast<int>(std::ceil(maxDelay)) + 4;
            powerBufferSize = nextPow2(reqSize);
            indexMask = powerBufferSize - 1;
            delayBuffer.assign(powerBufferSize, 0.f);
        }
        void reset() {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.f);
            writeIndex = 0;
        }
        JUCE_FORCEINLINE float processSample(float x, float lfoValue) {
            float targetDelay = baseDelay + depth * lfoValue;
            int d_int = static_cast<int>(targetDelay);
            float d_frac = targetDelay - static_cast<float>(d_int);
            bool hasFraction = (d_frac > 0.f);
            int offset = hasFraction ? 1 : 0;
            int index0 = (writeIndex - d_int - offset) & indexMask;
            float frac = hasFraction ? (1.f - d_frac) : 0.f;
            int index1 = (index0 + 1) & indexMask;
            float delayedV = (1.f - frac) * delayBuffer[index0] + frac * delayBuffer[index1];

            float v = x - coefficient * delayedV;
            float y = coefficient * v + delayedV;

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
        float baseDelay;
        float maxDelay;
        float coefficient;
        float depth;
        size_t lfoIndex;
        float sampleRate;
        std::vector<float> delayBuffer;
        int writeIndex;
        int powerBufferSize;
        int indexMask;
    };

} // namespace project
