#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
BasicGainAudioProcessorEditor::BasicGainAudioProcessorEditor (BasicGainAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p) {
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    mGainControlSlider.setBounds(0, 0, 100, 100);
    mGainControlSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    mGainControlSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);

    auto& params = processor.getParameters();
    AudioParameterFloat* gainParam = (AudioParameterFloat*)params.getUnchecked(0);
    mGainControlSlider.setRange(gainParam->range.start, gainParam->range.end);
    mGainControlSlider.setValue(*gainParam);
    mGainControlSlider.onValueChange = [this, gainParam] {
        *gainParam = mGainControlSlider.getValue();
    };

    mGainControlSlider.onDragStart = [gainParam] {
        gainParam->beginChangeGesture();
    };

    mGainControlSlider.onDragEnd = [gainParam] {
        (*gainParam).endChangeGesture();
    };

    addAndMakeVisible(mGainControlSlider);
}

BasicGainAudioProcessorEditor::~BasicGainAudioProcessorEditor() {

}

//==============================================================================
void BasicGainAudioProcessorEditor::paint (Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Basic Gain!", getLocalBounds(), Justification::centred, 1);
}

void BasicGainAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
