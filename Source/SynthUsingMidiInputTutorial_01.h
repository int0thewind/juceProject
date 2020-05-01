/*
  ==============================================================================

   This file is part of the JUCE tutorials.
   Copyright (c) 2017 - ROLI Ltd.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             SynthUsingMidiInputTutorial
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Synthesiser with midi input.

 dependencies:     juce_audio_basics, juce_audio_devices, juce_audio_formats,
                   juce_audio_processors, juce_audio_utils, juce_core,
                   juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics, juce_gui_extra
 exporters:        xcode_mac, vs2017, linux_make

 type:             Component
 mainClass:        MainContentComponent

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/


#pragma once

//==============================================================================
// Nothing in this SineWaveSound class as the
struct SineWaveSound   : public SynthesiserSound
{
    SineWaveSound() = default;

    bool appliesToNote    (int) override        { return true; }
    bool appliesToChannel (int) override        { return true; }
};

//==============================================================================
struct SineWaveVoice   : public SynthesiserVoice
{
    SineWaveVoice() = default;

    bool canPlaySound (SynthesiserSound* sound) override {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    /**
     * This function is called when the synthesiser started to play.
     * @param midiNoteNumber The midi note number that the synthesiser want to play
     * @param velocity The dynamic of the midi note that the synthesiser want to play
     */
    void startNote (int midiNoteNumber, float velocity, SynthesiserSound*, int /*currentPitchWheelPosition*/) override {
        this->currentAngle = 0.0;
        this->level = velocity * 0.15;
        this->tailOff = 0.0;

        double cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        double cyclesPerSample = cyclesPerSecond / this->getSampleRate();

        this->angleDelta = cyclesPerSample * 2.0 * MathConstants<double>::pi;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override {
        if (allowTailOff) {
            if (this->tailOff == 0.0) {
                this->tailOff = 1.0;
            }
        } else {
            this->clearCurrentNote();
            this->angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int) override      {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override {
        // This function only do works if the angleDelta is not zero.
        // It would be useless if we render the audio with no angleDelta
        if (this->angleDelta != 0.0) {
            if (this->tailOff > 0.0) {
                /* [7]
                 * When the key has been released the tailOff value will be greater than zero.
                 */
                while (--numSamples >= 0) {
                    auto currentSample = (float) (std::sin (this->currentAngle) * this->level * this->tailOff);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;) {
                        outputBuffer.addSample(i, startSample, currentSample);
                    }

                    this->currentAngle += this->angleDelta;
                    ++startSample;
                    /* [8]
                     * We use exponential decay envelope shape
                     */
                    this->tailOff *= 0.99;

                    if (this->tailOff <= 0.005) {
                        /* [9]
                         * We must call the clearCurrentNote() function to reset the voice
                         * and let it be ready to reuse.
                         */
                        this->clearCurrentNote();

                        this->angleDelta = 0.0;
                        break;
                    }
                }
            } else {
                /* [6]
                 * Normal state of the function
                 * see [7]
                 */
                while (--numSamples >= 0) {
                    auto currentSample = (float) (std::sin (this->currentAngle) * this->level);

                    for (int i = outputBuffer.getNumChannels(); --i >= 0;) {
                        outputBuffer.addSample(i, startSample, currentSample);
                    }

                    this->currentAngle += this->angleDelta;
                    ++startSample;
                }
            }
        }
    }

private:
    // These variables stores what values should be used for the current time to make the sine waves
    // tailOff is used to give each voice a slightly softer release to its amplitude envelope
    double currentAngle = 0.0, angleDelta = 0.0, level = 0.0, tailOff = 0.0;
};

//==============================================================================
class SynthAudioSource   : public AudioSource
{
public:
    explicit SynthAudioSource (MidiKeyboardState& keyState) : keyboardState (keyState) {
        /* [1]
         * The number of voices added determines the polyphony of the synthesiser.
         */
        for (int i = 0; i < 4; ++i) {       // [1]
            this->synth.addVoice(new SineWaveVoice());
        }

        /* [2]
         * We add the sound so that the synthesiser knows which sounds it can play
         */
        this->synth.addSound (new SineWaveSound());       // [2]
    }

    __unused void setUsingSineWaveSound() {
        this->synth.clearSounds();
    }

    void prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRate) override {
        /* [3]
         * Set the synthesiser's sample rate
         */
        this->synth.setCurrentPlaybackSampleRate (sampleRate); // [3]
    }

    void releaseResources() override {}

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override {
        bufferToFill.clearActiveBufferRegion();

        MidiBuffer incomingMidi;
        this->keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
                                             bufferToFill.numSamples, true);       // [4]

        this->synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
                               bufferToFill.startSample, bufferToFill.numSamples); // [5]
    }

private:
    MidiKeyboardState& keyboardState;
    Synthesiser synth;
};

//==============================================================================
class MainContentComponent   : public AudioAppComponent,
                               private Timer
{
public:
    MainContentComponent()
        : synthAudioSource  (this->keyboardState),
          keyboardComponent (this->keyboardState, MidiKeyboardComponent::horizontalKeyboard)
    {
        this->addAndMakeVisible (this->keyboardComponent);
        this->setAudioChannels (0, 2);

        this->setSize (600, 160);
        this->startTimer (400);
        this->keyboardComponent.setOctaveForMiddleC(4);
    }

    ~MainContentComponent() override {
        // Since this main component is directly inherit from AudioAppComponent which already have an audioDeviceManager
        // We don't need to manually set up everything and manage everything after
        this->shutdownAudio();
    }

    void resized() override {
        this->keyboardComponent.setBounds (10, 10, this->getWidth() - 20, this->getHeight() - 20);
    }

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override {
        this->synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override {
        this->synthAudioSource.getNextAudioBlock (bufferToFill);
    }

    void releaseResources() override {
        this->synthAudioSource.releaseResources();
    }

private:
    void timerCallback() override {
        this->keyboardComponent.grabKeyboardFocus();
        this->stopTimer();
    }

    //==========================================================================
    SynthAudioSource synthAudioSource;
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent keyboardComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};
