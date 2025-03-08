#pragma once
#include <JuceHeader.h>
#include "src/MyReverbConfig.h"
#include "src/MultiStageReverb.h"
#include "src/StageReverb.h"
#include "src/StageStereoizer.h"
#include "src/ReverbCommon.h"

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
                    // Build a mono input
                    float mono = 0.5f * (leftChannelData[i] + rightChannelData[i]);

                    // Pass through the reverb engine
                    float outMono = reverbEngine.processSample(mono);

                    // Let the stereoizer use the global LFO outputs
                    stereoizer.setGlobalLfoOutputsPointer(reverbEngine.globalLfoValues.data());

                    // Process into stereo
                    float l, r;
                    stereoizer.processSample(outMono, l, r);

                    leftChannelData[i] = l;
                    rightChannelData[i] = r;
                }
            }

            // New method to update the global size parameter
            void updateGlobalSizeParameter(float newSize) {
                reverbEngine.updateGlobalSizeParameter(newSize);
            }

        private:
            double sampleRate;
            MyEngine reverbEngine;
            multistage::StageStereoizer stereoizer;
        };

        //---------------------------------------------
        // Our node usage
        AudioReverb monoReverb;
        float globalSizeParam = 1.0f; // Default global size parameter

        void prepare(PrepareSpecs specs)
        {
            monoReverb.prepare(specs.sampleRate);
        }

        void reset()
        {
            monoReverb.reset();
        }

        // Process each audio block
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

        // Parameter handling: adding global size parameter at index 4.
        template <int P>
        void setParameter(double v)
        {
            if (P == 4) {
                globalSizeParam = static_cast<float>(v);
                monoReverb.updateGlobalSizeParameter(globalSizeParam);
            }
            // Additional parameters can be handled for other indices as needed.
        }

        void createParameters(ParameterDataList& data)
        {
            parameter::data p("Global Size", { 0.1, 2.0, 0.01 });
            registerCallback<4>(p);
            p.setDefaultValue(1.0);
            data.add(std::move(p));
        }

        void handleHiseEvent(HiseEvent& e) {}
        template <typename FrameDataType>
        void processFrame(FrameDataType& data) {}
    };

} // namespace project
