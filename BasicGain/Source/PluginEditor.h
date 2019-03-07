#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/**
 */
class BasicGainAudioProcessorEditor  : public AudioProcessorEditor {

public:
    BasicGainAudioProcessorEditor (BasicGainAudioProcessor&);
    ~BasicGainAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:

    Slider mGainControlSlider;
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BasicGainAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicGainAudioProcessorEditor)

};

