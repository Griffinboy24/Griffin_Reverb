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
    // Now, baseDelay and depth are specified in samples (at 44100 Hz)
    // and converted internally to milliseconds.
    // The extra smoothing layer has been removed so that the modulated delay time
    // is computed directly as: targetDelay = baseDelayMs + depth * lfoValue.
    //==============================================================================
    class SimpleAP {
    public:
        // Default constructor.
        SimpleAP()
            : baseDelayMs(0.f), maxDelayMs(0.f), coefficient(0.f), depth(0.f), lfoIndex(0),
            sampleRate(44100.f), writeIndex(0),
            factorDelay(0.f), powerBufferSize(0), indexMask(0)
        {
        }
        // Parameterized constructor.
        // The parameters baseDelay and d (depth) are provided in samples (assuming 44100 Hz)
        // and converted to milliseconds for internal processing.
        SimpleAP(float baseDelay, float coeff, float d, size_t lfoIdx)
            : coefficient(coeff), lfoIndex(lfoIdx),
            sampleRate(44100.f), writeIndex(0)
        {
            float refRate = 44100.f;
            // Convert sample count to milliseconds: (samples / 44100) * 1000.
            float convertedBaseDelay = (baseDelay * 1000.f) / refRate;
            float convertedDepth = (d * 1000.f) / refRate;
            baseDelayMs = convertedBaseDelay;
            depth = convertedDepth;
            maxDelayMs = baseDelayMs + 50.f + convertedDepth;
        }

        void prepare(float sr) {
            sampleRate = sr;
            writeIndex = 0;
            factorDelay = sampleRate / 1000.f;
            int reqSize = static_cast<int>(std::ceil((baseDelayMs + 50.f + depth) * factorDelay)) + 4;
            powerBufferSize = nextPow2(reqSize);
            indexMask = powerBufferSize - 1;
            delayBuffer.assign(powerBufferSize, 0.f);
        }
        void reset() {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.f);
            writeIndex = 0;
        }
        // processSample: Computes the modulated delay time directly without additional smoothing.
        JUCE_FORCEINLINE float processSample(float x, float lfoValue) {
            // Direct computation of target delay (in ms) from base delay and LFO modulation.
            float targetDelay = baseDelayMs + depth * lfoValue;
            float D = targetDelay * factorDelay;  // Convert ms delay to samples.
            int d_int = static_cast<int>(D);
            float d_frac = D - static_cast<float>(d_int);
            bool hasFraction = (d_frac > 0.f);
            int offset = hasFraction ? 1 : 0;
            int index0 = (writeIndex - d_int - offset) & indexMask;
            float frac = hasFraction ? (1.f - d_frac) : 0.f;
            int index1 = (index0 + 1) & indexMask;
            float delayedV = (1.f - frac) * delayBuffer[index0] + frac * delayBuffer[index1];

            // Apply fixed coefficient.
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
        float baseDelayMs;
        float maxDelayMs;
        float coefficient;
        float depth;
        size_t lfoIndex;
        float sampleRate;
        std::vector<float> delayBuffer;
        int writeIndex;
        float factorDelay;
        int powerBufferSize;
        int indexMask;
    };

} // namespace project
