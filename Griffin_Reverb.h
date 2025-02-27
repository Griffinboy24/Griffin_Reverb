#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
#include <tuple>
#include <utility>

#include "src\MultistageReverb.h" // This header defines project::multistage::MultistageReverb

//------------------------------------------------------------------------------
// Helper: make_array
// Deduces the size of a std::array from its initializer list (pre-C++20).
//------------------------------------------------------------------------------
template <typename T, typename... Ts>
constexpr std::array<typename std::common_type<T, Ts...>::type, 1 + sizeof...(Ts)>
make_array(T t, Ts... ts) {
    return { t, ts... };
}

namespace project {

    //==============================================================================
    // Legacy configuration (kept for backward compatibility if needed)
    struct GriffinReverbConfig {
        inline static constexpr auto lfoFrequencies = make_array(1.5f);
        struct AP {
            float baseDelay;
            float coefficient;
            float depth;
            size_t lfoIndex;
            constexpr float maxDelay() const { return baseDelay + 50.f; }
        };
        inline static constexpr auto aps = make_array(
            AP{ 10.f, 0.5f, 0.3f, 0 }
        );
    };


    //==============================================================================
    // Original ModularReverb (legacy reverb engine)
    template <typename Config>
    class ModularReverb {
    public:
        static constexpr size_t numLFOs = std::tuple_size<decltype(Config::lfoFrequencies)>::value;
        static constexpr size_t numAPs = std::tuple_size<decltype(Config::aps)>::value;
        ModularReverb() {
            for (size_t i = 0; i < numLFOs; ++i)
                lfos[i] = SimpleLFO(Config::lfoFrequencies[i]);
            for (size_t i = 0; i < numAPs; ++i) {
                const auto& s = Config::aps[i];
                aps[i] = SimpleAP(s.baseDelay, s.coefficient, s.depth, s.lfoIndex);
            }
        }
        void prepare(float sr) {
            for (auto& lfo : lfos)
                lfo.prepare(sr);
            for (auto& ap : aps)
                ap.prepare(sr);
        }
        void reset() {
            for (auto& ap : aps)
                ap.reset();
        }
        JUCE_FORCEINLINE float processSample(float x) {
            std::array<float, numLFOs> lfoValues;
            for (size_t i = 0; i < numLFOs; ++i)
                lfoValues[i] = lfos[i].update();
            float output = x;
            for (size_t i = 0; i < numAPs; ++i) {
                size_t idx = aps[i].getLfoIndex();
                if (idx >= numLFOs)
                    idx = 0;
                output = aps[i].processSample(output, lfoValues[idx]);
            }
            return output;
        }
    private:
        std::array<SimpleLFO, numLFOs> lfos;
        std::array<SimpleAP, numAPs> aps;
    };

    //==============================================================================
    // New Multistage Engine Integration
    // We replace the legacy ModularReverb with our new MultistageReverb.


// Revised Node using the new multistage reverb engine.
    template <int NV>
    struct Griffin_Reverb : public data::base {
        SNEX_NODE(Griffin_Reverb);
        struct MetadataClass { SN_NODE_ID("Griffin_Reverb"); };

        static constexpr bool isModNode() { return false; }
        static constexpr bool isPolyphonic() { return NV > 1; }
        static constexpr bool hasTail() { return false; }
        static constexpr bool isSuspendedOnSilence() { return false; }
        static constexpr int getFixChannelAmount() { return 2; }

        static constexpr int NumTables = 0;
        static constexpr int NumSliderPacks = 0;
        static constexpr int NumAudioFiles = 0;
        static constexpr int NumFilters = 0;
        static constexpr int NumDisplayBuffers = 0;

        // AudioReverb: A wrapper that connects the new multistage reverb engine to the node.
        class AudioReverb {
        public:
            AudioReverb() : sampleRate(44100.f), reverb() {} // Default constructs MultistageReverb
            void prepare(double sr) {
                sampleRate = sr;
                reverb.prepare(static_cast<float>(sampleRate));
            }
            JUCE_FORCEINLINE float processSample(float x) { return reverb.processSample(x); }
            inline void process(float* samples, int numSamples) {
                for (int i = 0; i < numSamples; ++i)
                    samples[i] = reverb.processSample(samples[i]);
            }
            void reset() { reverb.reset(); }
        private:
            double sampleRate;
            // Use our new multistage reverb engine.
            multistage::MultistageReverb reverb;
        };

        AudioReverb monoReverb;
        std::vector<float> monoBuffer;

        void prepare(PrepareSpecs specs) {
            float sampleRate = specs.sampleRate;
            monoReverb.prepare(sampleRate);
            monoBuffer.resize(static_cast<size_t>(specs.blockSize));
        }
        void reset() { monoReverb.reset(); }
        template <typename ProcessDataType>
        void process(ProcessDataType& data) {
            auto& fixData = data.template as<ProcessData<getFixChannelAmount()>>();
            auto audioBlock = fixData.toAudioBlock();
            auto* leftChannelData = audioBlock.getChannelPointer(0);
            auto* rightChannelData = audioBlock.getChannelPointer(1);
            int blockSize = data.getNumSamples();
            FloatVectorOperations::copy(monoBuffer.data(), leftChannelData, blockSize);
            FloatVectorOperations::add(monoBuffer.data(), rightChannelData, blockSize);
            FloatVectorOperations::multiply(monoBuffer.data(), 0.5f, blockSize);
            for (int i = 0; i < blockSize; ++i)
                monoBuffer[i] = monoReverb.processSample(monoBuffer[i]);
            FloatVectorOperations::copy(leftChannelData, monoBuffer.data(), blockSize);
            FloatVectorOperations::copy(rightChannelData, monoBuffer.data(), blockSize);
        }
        void createParameters(ParameterDataList& data) {}
        void handleHiseEvent(HiseEvent& e) {}
        template <typename FrameDataType>
        void processFrame(FrameDataType& data) {}
    };

} // namespace project
