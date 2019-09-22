#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define MAX_DELAY_TIME 2

using namespace std;

//==============================================================================
/**
 */

class BasicDelayAudioProcessor : public AudioProcessor {

public:

    //==============================================================================
    BasicDelayAudioProcessor();
    ~BasicDelayAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    float lerp(float sampleX1, float sampleX2, float inPhase);

private:

    AudioParameterFloat* mDryWetParam;
    AudioParameterFloat* mFeedbackParam;
    AudioParameterFloat* mDelayTimeParam;

    float mDelayTimeSmoothed;

    float* mCircularBufferLeft;
    float* mCircularBufferRight;

    int mCircularBufferWriteHead;
    int mCircularBufferLength;

    // These are floating point values, because we want to use them to represent intersample values
    float mDelayTimeInSamples;
    float mDelayReadHead;

    float mFeedbackLeft;
    float mFeedbackRight;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BasicDelayAudioProcessor)
};
