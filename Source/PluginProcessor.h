#pragma once

#ifndef ULTRALIGHTJUCE_PLUGINPROCESSOR_H
#define ULTRALIGHTJUCE_PLUGINPROCESSOR_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <Ultralight/Ultralight.h>

#include "Ultralight/RefPtr.h"
#include "Ultralight/Renderer.h"
#include "Config.h"

//==============================================================================
class AudioPluginAudioProcessor  :
        public juce::AudioProcessor,
        public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;
    static ultralight::RefPtr<ultralight::Renderer> RENDERER;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};

#endif //ULTRALIGHTJUCE_PLUGINPROCESSOR_H
