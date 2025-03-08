#pragma once
#include <cmath>
#include "ReverbCommon.h"

namespace project {
    namespace multistage {

        // Fast exponential approximation using (1 + x/256)^256 via exponentiation by squaring.
        // Approximates exp(x) for small-to-moderate values of x.
        inline float fastExp(float x) {
            x = 1.0f + x / 256.0f;
            x *= x; x *= x; x *= x; x *= x;
            x *= x; x *= x; x *= x; x *= x;
            return x;
        }

        // Fast pow10 approximation.
        // Computes 10^(x) as exp(x * ln(10)); ln(10) ~ 2.302585093.
        inline float fastPow10(float x) {
            return fastExp(x * 2.302585093f);
        }

        // Inline tan approximation by Michael Massberg.
        inline float tanA(float x) {
            float x2 = x * x;
            return x * (0.999999492001f + x2 * -0.096524608111f) /
                (1.0f + x2 * (-0.429867256894f + x2 * 0.009981877999f));
        }

        // One-pole high-shelf filter for high-frequency attenuation.
        // The filter is designed such that:
        //   - At DC, H(1) = 1 (0 dB)
        //   - At high frequencies (z = -1), H(-1) = G = 10^(dBgain/20)
        // 
        // The coefficients are computed as:
        //   float G = 10^(dBgain/20)
        //   float K = tan(pi * cutoff / sampleRate)
        //   norm = 1/(1 + G*K)
        //   b0 = G*(1+K)*norm
        //   b1 = G*(K-1)*norm
        //   a1 = (G*K - 1)*norm
        //
        // This filter is implemented in direct form I:
        //   y[n] = b0 * x[n] + b1 * x[n-1] - a1 * y[n-1]
        //
        // For example, with dBgain = -6 dB, the high frequencies will be attenuated by 6 dB.
        class LexiconShelvingFilter {
        public:
            LexiconShelvingFilter()
                : b0(0.0f), b1(0.0f), a1(0.0f), x1(0.0f), y1(0.0f) {
            }

            // Set filter parameters.
            // cutoff: shelf cutoff frequency in Hz.
            // dBgain: desired high-frequency gain in dB (negative for cut, positive for boost).
            // sampleRate: sampling rate in Hz.
            void setParameters(float cutoff, float dBgain, float sampleRate) {
                // Calculate high-frequency gain factor G = 10^(dBgain/20).
                // For attenuation, dBgain is negative so G < 1.
                float G = fastPow10(dBgain / 20.0f);
                // Prewarp the cutoff frequency using our tan approximation.
                float K = tanA(M_PI * cutoff / sampleRate);
                // Compute normalization factor.
                float norm = 1.0f / (1.0f + G * K);
                // Compute the filter coefficients.
                b0 = G * (1.0f + K) * norm;
                b1 = G * (K - 1.0f) * norm;
                a1 = (G * K - 1.0f) * norm;
            }

            // Process one sample. Call this per-sample in your audio loop.
            inline float processSample(float x) {
                float y = b0 * x + b1 * x1 - a1 * y1;
                x1 = x;
                y1 = y;
                return y;
            }

            // Reset the filter state (e.g., on initialization or when switching presets).
            void reset() {
                x1 = 0.0f;
                y1 = 0.0f;
            }

        private:
            float b0, b1, a1;
            float x1, y1;
        };

    } // namespace multistage
} // namespace project
