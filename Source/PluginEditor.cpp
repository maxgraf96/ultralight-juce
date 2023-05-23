#include "PluginEditor.h"

PluginAudioProcessorEditor::PluginAudioProcessorEditor(AudioPluginAudioProcessor& processor)
        : juce::AudioProcessorEditor(&processor)
{
    // Initialize your GUI components here

    setSize(1024, 700); // Set an initial size for the plugin editor

    // Add the GUI main component
    guiMainComponent = std::make_unique<GUIMainComponent>(processor.parameters);
//    guiMainComponent = std::make_unique<GUIMainComponent>(*processor.renderer, processor.parameters);
    addAndMakeVisible(guiMainComponent.get());

    setResizable(true, true);
}

PluginAudioProcessorEditor::~PluginAudioProcessorEditor()
{
}

void PluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Paint the plugin editor background
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PluginAudioProcessorEditor::resized()
{
    if(guiMainComponent.get())
        guiMainComponent->resized();
}
