#pragma once

#ifndef ULTRALIGHTJUCE_PLUGINEDITOR_H
#define ULTRALIGHTJUCE_PLUGINEDITOR_H

#include "GUIMainComponent.h"
#include <JuceHeader.h>
#include <memory>
#include "PluginProcessor.h"

class PluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    PluginAudioProcessorEditor(AudioPluginAudioProcessor& processor);
    ~PluginAudioProcessorEditor();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    std::unique_ptr<GUIMainComponent> guiMainComponent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessorEditor)
};

#endif //ULTRALIGHTJUCE_PLUGINEDITOR_H