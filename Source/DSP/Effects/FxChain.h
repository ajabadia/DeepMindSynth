#include "Processors/Distortion.h"
#include "Processors/DeepMindChorus.h"
#include "Processors/DeepMindPhaser.h"
#include "Processors/DeepMindEQ.h"
#include "Processors/DeepMindDelay.h"
#include "Processors/DeepMindReverb.h"

namespace DeepMindDSP
{
    class FxChain
    {
    public:
        FxChain();
        
        void prepare(const juce::dsp::ProcessSpec& spec);
        void reset();
        
        void process(juce::dsp::AudioBlock<float>& block);
        
        // Effect parameters
        void setDistortionParams(float drive, float tone, float mix, int type);
        void setPhaserParams(float rate, float depth, float feedback, float mix);
        void setChorusParams(float rate, float depth, float mix);
        void setDelayParams(float time, float feedback, float mix);
        void setReverbParams(float size, float damp, float mix);
        void setEQParams(float lg, float lf, float lmg, float lmf, float lmq, float hmg, float hmf, float hmq, float hg, float hf);
        
        void setRoutingMode(int mode); // 0=Series, 1=Parallel

    private:
        Distortion distortion;
        DeepMindPhaser phaser;
        DeepMindChorus chorus;
        DeepMindDelay delay;
        DeepMindReverb reverb;
        DeepMindEQ eq;
        
        int currentRouting = 0; // 0=Series
        
        // Parallel Processing Buffers
        juce::AudioBuffer<float> parBuffer;
        juce::AudioBuffer<float> accBuffer;
        
        double sampleRate = 44100.0;
    };
}
