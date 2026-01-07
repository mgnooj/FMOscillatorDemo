/*
  ==============================================================================

    FMOscillator.h
    Created:    25 Nov 2025 11:00:00am
    Modified:   2 Jan 2026 12:17:00pm

  ==============================================================================
*/

#pragma once
#include <cmath>

struct FMOscillatorSound final : public SynthesiserSound
{
    bool appliesToNote (int /*midiNoteNumber*/) override    { return true; }
    bool appliesToChannel (int /*midiChannel*/) override    { return true; }
};

// Classic FM operator with two sine wave oscillators in series
// Modulator osc output -> Carrier osc input
struct FMOscillatorVoice final : public SynthesiserVoice
{
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        carrierPhaseIndex = 0.0;
        modulatorPhaseIndex = 0.0;
        auto carrierFreq = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = carrierFreq / getSampleRate();
        carrierPhaseIncrement = cyclesPerSample * MathConstants<double>::twoPi;
        modPhaseIncrement = carrierPhaseIncrement * modRatio;
    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! approximatelyEqual (carrierPhaseIncrement, 0.0))
        {
            while (--numSamples >= 0)
            {
                // Increment mod phase
                modulatorPhaseIndex += modPhaseIncrement;
                auto modWaveValue = std::sin(modulatorPhaseIndex);

                // Apply modulator to carrier
                carrierPhaseIndex += carrierPhaseIncrement + (modWaveValue * modIndex);
                auto carrierWaveValue = std::sin(carrierPhaseIndex);
                auto levelAdjustedSample = carrierWaveValue * level;

                // Write sample to output buffers
                auto outputLen = outputBuffer.getNumChannels();
                for (auto channel = 0; channel < outputLen; ++channel)
                    outputBuffer.addSample(channel, startSample, levelAdjustedSample);

                // Increment sample index
                startSample++;
            }
        }
    }

    bool canPlaySound (SynthesiserSound* sound) override { return dynamic_cast<FMOscillatorSound*> (sound) != nullptr; }
    void stopNote (float /*velocity*/, bool /*allowTailOff*/) override { carrierPhaseIncrement = 0.0; }
    void pitchWheelMoved (int /*newValue*/) override                              {}
    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override    {}

    using SynthesiserVoice::renderNextBlock;

    double modulatorPhaseIndex = 0.0, modPhaseIncrement = 0.0;
    double carrierPhaseIndex = 0.0, carrierPhaseIncrement = 0.0;
    double modIndex = 0.0, modRatio = 0.0;
    double level = 1.0;
};
