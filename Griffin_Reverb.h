#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <algorithm>
#include <vector>
#include <tuple>
#include <utility>

#include "src\MultistageReverb.h" // Uses the new compile?time multistage engine

namespace project {

    //==============================================================================
    // New Griffin_Reverb Node using the fully compile?time configured reverb engine.
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

        // AudioReverb: A lightweight wrapper around our compile?time reverb engine.
        class AudioReverb {
        public:
            AudioReverb() : sampleRate(44100.f), reverb() {}
            void prepare(double sr) {
                sampleRate = sr;
                reverb.prepare(static_cast<float>(sampleRate));
            }
            // Inlined processing: the chain is fully unrolled at compile time.
            JUCE_FORCEINLINE float processSample(float x) { return reverb.processSample(x); }
            inline void process(float* samples, int numSamples) {
                for (int i = 0; i < numSamples; ++i)
                    samples[i] = reverb.processSample(samples[i]);
            }
            void reset() { reverb.reset(); }
        private:
            double sampleRate;
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
