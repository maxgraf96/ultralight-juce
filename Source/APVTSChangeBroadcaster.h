//
// Created by Max on 23/05/2023.
//

#ifndef ULTRALIGHTJUCE_APVTSCHANGEBROADCASTER_H
#define ULTRALIGHTJUCE_APVTSCHANGEBROADCASTER_H

#include <JuceHeader.h>

class APVTSChangeBroadcaster :
        public juce::AudioProcessorValueTreeState::Listener,
        public juce::ChangeBroadcaster
        {
public:
    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        // This method will be called when a parameter value changes
        // Perform your desired actions here
        // ...

        // Notify listeners of the parameter change
        sendChangeMessage();
    }

};
#endif //ULTRALIGHTJUCE_APVTSCHANGEBROADCASTER_H
