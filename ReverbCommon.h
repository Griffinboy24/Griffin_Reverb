#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>

//------------------------------------------------------------------------------
// Fallback definition for JUCE_FORCEINLINE if not already defined.
#ifndef JUCE_FORCEINLINE
#if defined(_MSC_VER)
#define JUCE_FORCEINLINE __forceinline
#else
#define JUCE_FORCEINLINE inline __attribute__((always_inline))
#endif
#endif

//------------------------------------------------------------------------------
// Helper: ms_make_array
// A renamed helper to deduce the size of a std::array from its initializer list.
template <typename T, typename... Ts>
constexpr std::array<typename std::common_type<T, Ts...>::type, 1 + sizeof...(Ts)>
ms_make_array(T t, Ts... ts) {
    return { t, ts... };
}

namespace project {

    //==============================================================================
    // Simple LFO Class
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
    //==============================================================================
    class SimpleAP {
    public:
        // Default constructor.
        SimpleAP()
            : baseDelayMs(0.f), maxDelayMs(50.f), coefficient(0.f), depth(0.f), lfoIndex(0),
            sampleRate(44100.f), smoothedDelay(0.f), smoothedCoeff(0.f), writeIndex(0),
            factorDelay(0.f), powerBufferSize(0), indexMask(0)
        {
        }
        // Parameterized constructor.
        SimpleAP(float baseDelay, float coeff, float d, size_t lfoIdx)
            : baseDelayMs(baseDelay), coefficient(coeff), depth(d), lfoIndex(lfoIdx),
            sampleRate(44100.f), writeIndex(0)
        {
            maxDelayMs = baseDelayMs + 50.f;
            smoothedDelay = baseDelayMs;
            smoothedCoeff = coefficient;
        }

        void prepare(float sr) {
            sampleRate = sr;
            smoothedDelay = baseDelayMs;
            smoothedCoeff = coefficient;
            writeIndex = 0;
            factorDelay = sampleRate / 1000.f;
            int reqSize = static_cast<int>(std::ceil(maxDelayMs * factorDelay)) + 4;
            powerBufferSize = nextPow2(reqSize);
            indexMask = powerBufferSize - 1;
            delayBuffer.assign(powerBufferSize, 0.f);
        }
        void reset() {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.f);
            writeIndex = 0;
            smoothedDelay = baseDelayMs;
            smoothedCoeff = coefficient;
        }
        JUCE_FORCEINLINE float processSample(float x, float lfoValue) {
            float targetCoeff = coefficient + depth * lfoValue;
            targetCoeff = std::clamp(targetCoeff, -0.99f, 0.99f);
            constexpr float smoothingFactor = 0.01f;
            float oneMinusSmooth = 1.f - smoothingFactor;
            smoothedCoeff = oneMinusSmooth * smoothedCoeff + smoothingFactor * targetCoeff;
            smoothedDelay = oneMinusSmooth * smoothedDelay + smoothingFactor * baseDelayMs;

            float D = smoothedDelay * factorDelay;
            int d_int = static_cast<int>(D);
            float d_frac = D - static_cast<float>(d_int);
            bool hasFraction = (d_frac > 0.f);
            int offset = hasFraction ? 1 : 0;
            int index0 = (writeIndex - d_int - offset) & indexMask;
            float frac = hasFraction ? (1.f - d_frac) : 0.f;
            int index1 = (index0 + 1) & indexMask;
            float delayedV = (1.f - frac) * delayBuffer[index0] + frac * delayBuffer[index1];

            float v = x - smoothedCoeff * delayedV;
            float y = smoothedCoeff * v + delayedV;

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
        float baseDelayMs;
        float maxDelayMs;
        float coefficient;
        float depth;
        size_t lfoIndex;
        float sampleRate;
        float smoothedDelay;
        float smoothedCoeff;
        std::vector<float> delayBuffer;
        int writeIndex;
        float factorDelay;
        int powerBufferSize;
        int indexMask;
    };

} // namespace project
