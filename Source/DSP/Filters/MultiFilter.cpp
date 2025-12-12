#include "MultiFilter.h"

using namespace DeepMindDSP;

MultiFilter::MultiFilter()
{
}

MultiFilter::~MultiFilter()
{
}

void MultiFilter::prepare(const juce::dsp::ProcessSpec& spec)
{
    ladderFilter.prepare(spec);
    svFilter.prepare(spec);
    
    // Initialize waveshaper with basic tanh for now
    distortionSat.prepare(spec);
    distortionSat.functionToUse = [](float x) { return std::tanh(x); };
}

void MultiFilter::reset()
{
    ladderFilter.reset();
    svFilter.reset();
    distortionSat.reset();
}

void MultiFilter::process(juce::dsp::AudioBlock<float>& block)
{
    juce::dsp::ProcessContextReplacing<float> context(block);

    switch (currentType)
    {
        case FilterType::Jupiter:
        {
            // Classic Clean Ladder (24dB)
            ladderFilter.setMode(juce::dsp::LadderFilterMode::LPF24);
            ladderFilter.process(context);
            break;
        }

        case FilterType::MS20:
        {
            // Screaming 20: Input Saturation -> State Variable Filter
            // Apply slight drive before filter
            distortionSat.process(context);
            
            svFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            svFilter.process(context);
            break;
        }

        case FilterType::Acid303:
        {
            // Acid: Input Boost/Distortion -> Ladder Filter High Res
            // Boost input for character
            // (In a block context, simple gain is easiest applied via iterating or a gain processor, 
            // but for now let's reuse the saturator which limits peaks too).
            distortionSat.process(context);
            
            ladderFilter.setMode(juce::dsp::LadderFilterMode::LPF24); // 18dB not available, fallback to 24dB
            ladderFilter.process(context);
            break;
        }
    }
}

void MultiFilter::setType(FilterType type)
{
    currentType = type;
}

void MultiFilter::setCutoff(float frequency)
{
    // Clamp
    if (frequency < 20.0f) frequency = 20.0f;
    if (frequency > 20000.0f) frequency = 20000.0f;
    
    currentCutoff = frequency; // Store for getter
    
    ladderFilter.setCutoffFrequencyHz(frequency);
    svFilter.setCutoffFrequency(frequency);
}

float MultiFilter::getCutoff() const
{
    return currentCutoff;
}

void MultiFilter::setResonance(float resonance)
{
    // DeepMind 12 Calibration:
    // The original hardware can self-oscillate.
    // JUCE Ladder filter roughly self-oscillates at res = 1.0.
    // However, linear 0-1 control feels "weak" at low values.
    // Let's use a slight exponential curve for the knob response.
    
    float calibratedRes = resonance * resonance; // Simple taper
    
    // For Ladder (Jupiter/Acid)
    // Avoid exactly 1.0 to prevent blowing up in floating point if unstable, cap at 0.99 for safety unless we want pure sine.
    ladderFilter.setResonance(juce::jmap(calibratedRes, 0.0f, 0.99f));
    
    // For SVF (MS-20)
    // SVF Q factor: 1/sqrt(2) is flat (~0.707).
    // High Q can go up to 10 or 20.
    // resonance input is 0-1.
    // Map 0 -> 0.707, 1 -> 10.0
    svFilter.setResonance(juce::jmap(calibratedRes, 0.707f, 10.0f));
}

void MultiFilter::setDrive(float drive)
{
    // Ladder filter drive
    ladderFilter.setDrive(1.0f + (drive * 2.0f)); // 1.0 to 3.0 range
    
    // Distortion Saturation gain
    // Tanh behaves differently based on input gain.
    // We can simulate drive by pre-gain.
    // Since we don't have a gain member, we can't easily set it here without modifying the class.
    // For now, Ladder Drive is the main parameter impacted by this.
}
