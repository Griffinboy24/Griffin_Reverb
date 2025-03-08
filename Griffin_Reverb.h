#pragma once
#include <JuceHeader.h>

#include "src\MyReverbConfig.h"
#include "src\MultiStageReverb.h"
#include "src\StageReverb.h"
#include "src\StageStereoizer.h"
#include "src\ReverbCommon.h"

namespace project {

    template <int NV>
    struct Griffin_Reverb : public data::base
    {
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

        //---------------------------------------------
        // Our compile-time engine plus stereoizer
        using MyEngine = multistage::MultiStageReverb<multistage::MyReverbConfig>;

        class AudioReverb {
        public:
            AudioReverb()
                : sampleRate(44100.0),
                reverbEngine(),
                stereoizer()
            {
            }

            void prepare(double sr)
            {
                sampleRate = sr;
                reverbEngine.prepare(static_cast<float>(sampleRate));
                stereoizer.prepare(static_cast<float>(sampleRate));
            }

            void reset()
            {
                reverbEngine.reset();
                stereoizer.reset();
            }

            // Process a block of samples
            void process(float* leftChannelData, float* rightChannelData, int numSamples)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    // build a mono input
                    float mono = 0.5f * (leftChannelData[i] + rightChannelData[i]);

                    // pass through the reverb
                    float outMono = reverbEngine.processSample(mono);

                    // Now let the stereoizer see the same global LFO outputs
                    // We can do that each sample, or once per block if we trust continuity
                    // For clarity, do it each sample here:
                    stereoizer.setGlobalLfoOutputsPointer(reverbEngine.globalLfoValues.data());

                    // process into stereo
                    float l, r;
                    stereoizer.processSample(outMono, l, r);

                    leftChannelData[i] = l;
                    rightChannelData[i] = r;
                }
            }

        private:
            double sampleRate;
            MyEngine reverbEngine;
            multistage::StageStereoizer stereoizer;
        };

        //---------------------------------------------
        // Our node usage
        AudioReverb monoReverb;

        void prepare(PrepareSpecs specs)
        {
            monoReverb.prepare(specs.sampleRate);
        }

        void reset()
        {
            monoReverb.reset();
        }

        // for each block
        template <typename ProcessDataType>
        void process(ProcessDataType& data)
        {
            auto& fixData = data.template as<ProcessData<getFixChannelAmount()>>();
            auto audioBlock = fixData.toAudioBlock();
            auto* leftChannelData = audioBlock.getChannelPointer(0);
            auto* rightChannelData = audioBlock.getChannelPointer(1);
            int blockSize = data.getNumSamples();

            monoReverb.process(leftChannelData, rightChannelData, blockSize);
        }

        void createParameters(ParameterDataList& data) {}
        void handleHiseEvent(HiseEvent& e) {}
        template <typename FrameDataType>
        void processFrame(FrameDataType& data) {}
    };

} // namespace project
