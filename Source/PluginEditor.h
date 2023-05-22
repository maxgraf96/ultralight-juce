#pragma once

#include "GUIMainComponent.h"
#include <JuceHeader.h>
#include <memory>

class PluginAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    PluginAudioProcessorEditor(juce::AudioProcessor& processor);
    ~PluginAudioProcessorEditor();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:

    std::unique_ptr<GUIMainComponent> guiMainComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAudioProcessorEditor)
};