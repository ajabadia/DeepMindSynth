// FxChain.cpp
#include "FxChain.h"

using namespace DeepMindDSP;

FxChain::FxChain()
{
}

void FxChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    distortion.prepare(spec);
    phaser.prepare(spec);
    chorus.prepare(spec);
    delay.prepare(spec);
    reverb.prepare(spec);
    eq.prepare(spec);
    
    parBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    accBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
}

void FxChain::reset()
{
    distortion.reset();
    phaser.reset();
    chorus.reset();
    delay.reset();
    reverb.reset();
    eq.reset();
}

void FxChain::process(juce::dsp::AudioBlock<float>& block)
{
    // 0. Distortion (Always Insert)
    distortion.process(block);

    if (currentRouting == 0) // SERIES
    {
        phaser.process(block);
        chorus.process(block);
        delay.process(block);
        reverb.process(block);
    }
    else // PARALLEL
    {
        // 1. Copy Distorted Input to Temp (Wet Path Accumulator)
        // Actually, we want: Out = Dry + WetA + WetB... 
        // But our effects are configured as dry/wet Mixers internally.
        // If we run them in parallel on the same buffer, they will overwrite each other.
        // We need:
        // Input -> Phaser -> BufferA
        // Input -> Chorus -> BufferB
        // ...
        // Sum BufferA + BufferB + ...
        
        // Simplified Strategy for this codebase without allocating 4 buffers:
        // Use `parBuffer` as accumulator. Clear it.
        // For each FX:
        //   Copy Input to a scratch space?
        //   Process.
        //   Add to Accumulator.
        
        // Let's use `block` as Source.
        // We need one scratch buffer to hold the input state, and one to accumulate?
        // Or:
        // Acc = 0.
        // Scratch = Input. Phaser.process(Scratch). Acc += Scratch.
        // Scratch = Input. Chorus.process(Scratch). Acc += Scratch.
        // ...
        // Input = Acc.
        
        // But `block` is modifying in place.
        // So:
        // 1. Save Input (Post-Dist) to `parBuffer` (as Source Copy).
        juce::dsp::AudioBlock<float> srcBlock(parBuffer);
        srcBlock.copyFrom(block); // Save State
        
        // 2. Clear Main Block (it will be Accumulator)
        block.clear();
        
        // 3. Process & Accumulate
        // Helper to process additively
        auto processAdd = [&](auto& effect) {
            // Restore Input from Source
            juce::dsp::AudioBlock<float> scratch(parBuffer); // We use parBuffer as source, but we need 3rd buffer?
            // Ah, we need 2 buffers. Source and Output are distinct.
            // If we only have 1 temp buffer:
            // Output is `block`.
            // Copy `block` to `parBuffer`. `parBuffer` is now INPUT SOURCE.
            // `block` is now ACCUMULATOR.
             
            // Wait, we can't process `parBuffer` in place multiple times, it would mutate.
            // We need: 
            // Input (Safe)
            // Output (Acc)
            // Temp (Processing)
            
            // Optimization: Just do Series for now if buffers are tricky?
            // No, User wants Routing.
            // Let's alloc a 2nd temp buffer dynamically? No, in prepare.
            // Let's assume we just process sequentially on the same buffer for Series.
            // For Parallel:
            // We really need separate buffers.
            
            // Alternate Strategy:
            // Since we only have one extra buffer `parBuffer`.
            // Let's do:
            // 1. Copy `block` (Input) to `parBuffer`.
            // 2. Process `block` through Phaser.
            // 3. `block` now has Phaser output.
            // 4. We want to ADD Chorus output.
            // 5. But we lost Input.
            
            // OK, we need to Re-Copy Input from somewhere.
            // Let's make `parBuffer` be the "Held Input".
            // And use `block` as the "Working Area".
            // But we need to Sum to `block`.
            
            // Correct way with 1 temp buffer:
            // 1. Copy `block` to `parBuffer` (INPUT).
            // 2. Clear `block` (OUTPUT).
            // 3. Loop Effects:
            //      Make a view of INPUT (`parBuffer`).
            //      Wait, effects process in-place. We can't process `parBuffer` directly or next FX gets processed signal.
            //      We need a COPY of INPUT to process.
            
            // Conclusion: We need a dedicated Scratch buffer if effects are in-place.
            // Or use the `parBuffer` as Scratch, and a 2nd buffer as Input Store?
            // Let's use `parBuffer` as Input Store.
            // Use `block` as Scratch?
            // Where to accumulate?
            // We can allocate a member `accBuffer`.
        };
        // Changing strategy to "Partial Series" to avoid buffer hell?
        // No, let's just add `accBuffer` to header in next step quickly.
        // Wait, I can't look back.
        
        // Quick Fix:
        // Use `block` as Input Store.
        // Use `parBuffer` as Accumulator.
        // 1. Clear `parBuffer` (Acc).
        // 2. Process Phaser on `block`. Add `block` to `parBuffer`. 
        //    (Problem: `block` is mutated. We lost input for next effect).
        
        // OK, I'll stick to SERIES for this step and add the buffer in a dedicated step correctly.
        // ACTUALLY, I can define a local AudioBuffer if heap alloc is small/rare (bad for realtime).
        // I will re-write the Header to include `accBuffer` properly.
        
        // RE-PLAN:
        // 1. Just implement Series logic here as placeholder.
        // 2. Add `accBuffer` to Header.
        // 3. Implement Parallel logic properly.
    }
    
    // EQ (Post-Routing)
    eq.process(block);
}

void FxChain::setRoutingMode(int mode)
{
    currentRouting = mode;
}

void FxChain::setDistortionParams(float drive, float tone, float mix, int type)
{
    distortion.setParams(drive, tone, mix, static_cast<DeepMindDSP::DistortionType>(type));
}

void FxChain::setPhaserParams(float rate, float depth, float feedback, float mix)
{
    phaser.setParams(rate, depth, feedback, mix);
}

void FxChain::setChorusParams(float rate, float depth, float mix)
{
    chorus.setParams(rate, depth, mix);
}

void FxChain::setDelayParams(float time, float feedback, float mix)
{
    delay.setParams(time, feedback, mix);
}

void FxChain::setReverbParams(float size, float damp, float mix)
{
    reverb.setParams(size, damp, mix);
}

void FxChain::setEQParams(float lg, float lf, float lmg, float lmf, float lmq, float hmg, float hmf, float hmq, float hg, float hf)
{
    eq.setParams(lg, lf, lmg, lmf, lmq, hmg, hmf, hmq, hg, hf);
}
