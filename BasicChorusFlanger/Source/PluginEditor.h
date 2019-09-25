#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
/**
 */
class BasicChorusFlangerAudioProcessorEditor : public AudioProcessorEditor {

public:

    BasicChorusFlangerAudioProcessorEditor (BasicChorusFlangerAudioProcessor&);
    ~BasicChorusFlangerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    BasicChorusFlangerAudioProcessor& processor;

    Slider mDryWetSlider;
    Slider mDepthSlider;
    Slider mRateSlider;
    Slider mPhaseOffsetSlider;
    Slider mFeedbackSlider;

    ComboBox mType;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicChorusFlangerAudioProcessorEditor)

};

