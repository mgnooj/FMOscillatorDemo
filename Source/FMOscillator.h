/*
  ==============================================================================

    FMOscillator.h
    Created:    25 Nov 2025 11:00:00am

  ==============================================================================
*/

#pragma once
#include <cmath>

struct FMOscillatorSound final : public SynthesiserSound
{
    bool appliesToNote (int /*midiNoteNumber*/) override    { return true; }
    bool appliesToChannel (int /*midiChannel*/) override    { return true; }
};

// Classic FM phase modulation operator with two sine wave oscillators in series
// Modulator osc output -> Carrier osc phase
struct FMOscillatorVoice final : public SynthesiserVoice
{
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        double carrierFreq = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        double cyclesPerSample = carrierFreq / getSampleRate();
        carrierPhaseIncrement = cyclesPerSample * juce::MathConstants<double>::twoPi;
        modulatorPhaseIncrement = carrierPhaseIncrement * modulatorRatio;
    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (! approximatelyEqual (carrierPhaseIncrement, 0.0))
        {
            while (--numSamples >= 0)
            {
                // Increment mod phase and calculate mod wave
                modulatorPhaseIndex.advance(modulatorPhaseIncrement);
                double modulatorWaveValue = std::sin(modulatorPhaseIndex.phase);

                // Increment carrier phase and apply modulator to carrier
                carrierPhaseIndex.advance(carrierPhaseIncrement);
                double modulatedPhase = carrierPhaseIndex.phase + (modulatorWaveValue * modulatorIndex);
                double carrierWaveValue = std::sin(modulatedPhase);
                float levelAdjustedSample = static_cast<float>(carrierWaveValue * level);

                // Write sample to output buffers
                auto outputLen = outputBuffer.getNumChannels();
                for (auto channel = 0; channel < outputLen; ++channel)
                    outputBuffer.addSample(channel, startSample, levelAdjustedSample);

                // Increment sample index
                startSample++;
            }
        }
    }

    bool canPlaySound (SynthesiserSound* sound) override { 
        return dynamic_cast<FMOscillatorSound*> (sound) != nullptr; 
    }
    void stopNote (float /*velocity*/, bool /*allowTailOff*/) override { 
        carrierPhaseIncrement = 0.0;
    }
    void pitchWheelMoved (int /*newValue*/) override                              {}
    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override    {}

    using SynthesiserVoice::renderNextBlock;

    double carrierPhaseIncrement = 0.0, modulatorPhaseIncrement = 0.0;
    juce::dsp::Phase<double> carrierPhaseIndex { 0.0 }, modulatorPhaseIndex { 0.0 };
    double modulatorRatio = 1.0, modulatorIndex = 0.0;
    double level = 1.0;
};
