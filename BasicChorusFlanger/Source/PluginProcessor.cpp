#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BasicChorusFlangerAudioProcessor::BasicChorusFlangerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
#endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
#endif
                       )
#endif
{
    // User-controllable parameters
    addParameter(mDryWetParam = new AudioParameterFloat("drywet", "Dry / Wet", 0.0, 1.0, 0.5));
    addParameter(mDepthParam = new AudioParameterFloat("depth", "Depth", 0.0, 1.0, 0.5));
    addParameter(mRateParam = new AudioParameterFloat("rate", "Rate", 0.1f, 20.f, 10.f));
    addParameter(mPhaseOffsetParam = new AudioParameterFloat("phaseoffset", "Phase Offset", 0.0f, 1.0f, 0.f));
    addParameter(mFeedbackParam = new AudioParameterFloat("feedback", "Feedback", 0, 0.98, 0.5));
    addParameter(mTypeParam = new AudioParameterInt("type", "Type", 0, 1, 0));

    mCircularBufferLeft = nullptr;
    mCircularBufferRight = nullptr;
    mCircularBufferWriteHead = 0;
    mCircularBufferLength = 0;
    mLFOPhase = 0;

    mFeedbackLeft = 0;
    mFeedbackRight = 0;
}

BasicChorusFlangerAudioProcessor::~BasicChorusFlangerAudioProcessor() {
    if (mCircularBufferLeft != nullptr) {
        delete [] mCircularBufferLeft;
        mCircularBufferLeft = nullptr;
    }

    if (mCircularBufferRight != nullptr) {
        delete [] mCircularBufferRight;
        mCircularBufferRight = nullptr;
    }
}

//==============================================================================
const String BasicChorusFlangerAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool BasicChorusFlangerAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool BasicChorusFlangerAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool BasicChorusFlangerAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double BasicChorusFlangerAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int BasicChorusFlangerAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int BasicChorusFlangerAudioProcessor::getCurrentProgram() {
    return 0;
}

void BasicChorusFlangerAudioProcessor::setCurrentProgram (int index) {
}

const String BasicChorusFlangerAudioProcessor::getProgramName (int index) {
    return {};
}

void BasicChorusFlangerAudioProcessor::changeProgramName (int index, const String& newName) {
}

//==============================================================================
void BasicChorusFlangerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    mCircularBufferLength = sampleRate * MAX_DELAY_TIME;
    mLFOPhase = 0;

    if (mCircularBufferLeft == nullptr) {
        delete [] mCircularBufferLeft;
        mCircularBufferLeft = new float[mCircularBufferLength];
    }

    // Clear the buffers after instantiating!
    zeromem(mCircularBufferLeft, mCircularBufferLength * sizeof(float));

    if (mCircularBufferRight == nullptr) {
        delete [] mCircularBufferRight;
        mCircularBufferRight = new float[mCircularBufferLength];
    }

    // Clear the buffers after instantiating!
    zeromem(mCircularBufferRight, mCircularBufferLength * sizeof(float));

    mCircularBufferWriteHead = 0;
}

void BasicChorusFlangerAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BasicChorusFlangerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void BasicChorusFlangerAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) {
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

//    DBG("Dry/Wet: " << *mDryWetParam);
//    DBG("Depth: " << *mDepthParam);
//    DBG("Rate: " << *mRateParam);
//    DBG("Phase: " << *mPhaseOffsetParam);
//    DBG("Feedback: " << *mFeedbackParam);
//    DBG("Type: " << (int)*mTypeParam);

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear (i, 0, buffer.getNumSamples());
    }

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); i++) {
        float lfoOutLeft = sin(2 * M_PI * mLFOPhase);
        float lfoPhaseRight = mLFOPhase + *mPhaseOffsetParam;

        if (lfoPhaseRight > 1) {
            lfoPhaseRight -= 1;
        }

        float lfoOutRight = sin(2 * M_PI * lfoPhaseRight);

        lfoOutLeft *= *mDepthParam;
        lfoOutRight *= *mDepthParam;

        float lfoOutMappedLeft = 0;
        float lfoOutMappedRight = 0;

        // Chorus Effect
        if (*mTypeParam == 0) {
            lfoOutMappedLeft = jmap(lfoOutLeft, -1.0f, 1.0f, 0.005f, 0.03f);
            lfoOutMappedRight = jmap(lfoOutRight, -1.0f, 1.0f, 0.005f, 0.03f);
        } else {
            // Flanger Effect
            lfoOutMappedLeft = jmap(lfoOutLeft, -1.0f, 1.0f, 0.001f, 0.005f);
            lfoOutMappedRight = jmap(lfoOutRight, -1.0f, 1.0f, 0.001f, 0.005f);
        }

        // Map the LFO value to a range of time values (5 - 30ms)
        float delayTimeSamplesLeft = getSampleRate() * lfoOutMappedLeft;
        float delayTimeSamplesRight = getSampleRate() * lfoOutMappedRight;

        mLFOPhase += *mRateParam / getSampleRate();

        // Ensure the LFO phase is bounded between 0 and 1.
        if (mLFOPhase > 1) {
            mLFOPhase -= 1;
        }

        mCircularBufferLeft[mCircularBufferWriteHead] = leftChannel[i] + mFeedbackLeft;
        mCircularBufferRight[mCircularBufferWriteHead] = rightChannel[i] + mFeedbackRight;

        float delayReadHeadLeft = mCircularBufferWriteHead - delayTimeSamplesLeft;

        if (delayReadHeadLeft < 0) {
            delayReadHeadLeft += mCircularBufferLength;
        }

        float delayReadHeadRight = mCircularBufferWriteHead - delayTimeSamplesRight;

        if (delayReadHeadRight < 0) {
            delayReadHeadRight += mCircularBufferLength;
        }

        int readHeadLeft_x = (int) delayReadHeadLeft;
        int readHeadLeft_x1 = readHeadLeft_x + 1;
        float readHeadFloatLeft = delayReadHeadLeft - readHeadLeft_x;

        if (readHeadLeft_x1 >= mCircularBufferLength) {
            readHeadLeft_x1 -= mCircularBufferLength;
        }

        int readHeadRight_x = (int) delayReadHeadRight;
        int readHeadRight_x1 = readHeadRight_x + 1;
        float readHeadFloatRight = delayReadHeadRight - readHeadRight_x;

        if (readHeadRight_x1 >= mCircularBufferLength) {
            readHeadRight_x1 -= mCircularBufferLength;
        }

        float delaySampleLeft = lerp(mCircularBufferLeft[readHeadLeft_x], mCircularBufferLeft[readHeadLeft_x1], readHeadFloatLeft);
        float delaySampleRight = lerp(mCircularBufferRight[readHeadRight_x], mCircularBufferRight[readHeadRight_x1], readHeadFloatRight);

        mFeedbackLeft = *mFeedbackParam * delaySampleLeft;
        mFeedbackRight = *mFeedbackParam * delaySampleRight;

        mCircularBufferWriteHead++;

        // Write back into the sample with the half-second delayed signal
        buffer.setSample(0, i, buffer.getSample(0, i) *  (1 - *mDryWetParam) + delaySampleLeft * *mDryWetParam);
        buffer.setSample(1, i, buffer.getSample(1, i) *  (1 - *mDryWetParam) + delaySampleRight * *mDryWetParam);

        if (mCircularBufferWriteHead >= mCircularBufferLength) {
            mCircularBufferWriteHead = 0;
        }
    }
}

//==============================================================================
bool BasicChorusFlangerAudioProcessor::hasEditor() const {
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* BasicChorusFlangerAudioProcessor::createEditor() {
    return new BasicChorusFlangerAudioProcessorEditor (*this);
}

//==============================================================================
void BasicChorusFlangerAudioProcessor::getStateInformation (MemoryBlock& destData) {
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void BasicChorusFlangerAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new BasicChorusFlangerAudioProcessor();
}

float BasicChorusFlangerAudioProcessor::lerp(float sampleA, float sampleB, float inPhase) {
    return (1 - inPhase) * sampleA + inPhase * sampleB;
}
