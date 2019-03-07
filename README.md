# Programming Audio Plugins

Diving into audio plugin development and DSP. Yay!

---

## Digital Audio Theory

How do we store sound information inside the computer?

**Continuous Time Domain –** Analog waveforms are continuous, meaning that they hold an infinite amount of information. Two points A and B hold an infinite amount of information.

![](img/-0527a53b-d9d2-4bca-bb0a-e7b0ebc4d56funtitled)

**Discrete Time Domain –** Digital waveforms are discrete, so they hold a finite amount of information. Digital waveforms are recreations, or mere representations of their analog counterparts. It is physically impossible to represent the signal continuously, so instead we represent it as a discrete value at specific time intervals. Between two points A and B, we only have information that has been recorded at specific time intervals, no more, no less.

![](img/-dc20b5be-c6fe-4363-a39a-4859ba17fb48untitled)

This brings us to sampling theory and analog to digital conversion. How are we taking in discrete digital signals when working with audio? For example, let's say we're talking out loud in a room with a microphone in front of us. The human voice generates a continuous time signal which the mic picks up and converts to a continuous analog electrical signal, which is sent to the computer. The computer samples the signal at regular intervals (this is the A → D conversion).

Additionally vocabulary ...

**Sampling Rate:** Number of samples collected each second, which can be used to represent a soundwave digitally. The most common sample rate is 44.1kHz. For every 1 second of audio, there are 44,100 discrete samples which describe the value of each position of time inside of that signal. This number comes from the _Nyquist Shannon Sampling Theorem._ It states that in order to digitally store and reconstruct a signal, the sample rate of that signal must be at least double the highest frequency within the sampled signal. The human hearing range is considered to be between 20Hz to 22.5kHz, therefore we need to be sampling at a minimum of 44.1kHz without any frequency loss.

![](img/-572f71cd-5715-4ace-bf50-6254e8a7e6e3untitled)

**Block Sizes / Buffer:** Our audio applications can't simply take audio in and output it. They need to accomodate for other inputs as well (like MIDI controllers). Generally, it is known that having smaller block sizes means less latency when playing a MIDI keyboard, but why? Block size determines how many samples of the current smapling rate are processed at once. Think about this, if all our apps did was process incoming samples, it would have no time to check for MIDI or other incoming signals. We break up the process of the sample rate, by assigning a block size to the audio processing operation. The most commonly used sample rate is 512. When an audio plugin is running at a sampling rate of 44.1kHz and a 512 block size, it means that every second 44,100 samples of audio data is being processed. Within that one second of processing, the 44,100 samples are broken down into blocks of 512. The more audio that the processor can go through at once, the faster the process block will happen, because the processing has no interruptions. The block size of 512 means that every 512 samples, the audio engine will stop processing audio to do other things, like checking for MIDI or audio inputs. This is why block size extremely important in live settings, because the input needs to have minimum latency for the musician to feel the responsiveness of their MIDI instrument. When the block size is larger, there is more latency, but the overall CPU usage of the process is lessened.

![](img/-e5fa4f29-c60c-48d9-8a13-293f8e25e75buntitled)

Here is Ableton Live 10's Audio Preferences panel where we can see the Sample Rate and Buffer Size in action.

![](img/untitled-5ba15ef7-aff3-4ed5-956d-3394cc2a39dc.png)

---

## JUCE

JUCE is a C++ toolkit to create cross-platform audio plugins (native or VST/AAX/Audio Unit formats) and GUI applications.

[JUCE](https://juce.com/)

![](img/-9cf8f4db-9d2b-4eab-9d3b-8d5ec8a06c63untitled)

The Projucer is an IDE tool for creating and managing JUCE projects and allows configuration of project settings. When the files and settings for a JUCE project have been specified, the Projucer automatically generates a collection of 3rd-party project files to allow the project to be compiled natively on each target platform. It can currently generate Xcode projects, Visual Studio projects, Linux Makefiles, Android Ant builds and CodeBlocks projects. As well as providing a way to manage a project's files and settings, it also has a code editor, an integrated GUI editor, wizards for creating new projects and files, and a live coding engine useful for user interface design.

---

## Plugin Architecture

Threads are a very common topic when it comes to programming audio plugins.

> In computer science, a thread of execution is the smallest sequence of programmed instructions that can be managed independently by a scheduler, which is typically a part of the operating system. The implementation of threads and processes differs between operating systems, but in most cases a thread is a component of a process.

![](img/-b740854a-20d8-41d3-a4f0-bd4df441e3e1untitled)

In the diagram above, we see two objects labeled as threads. The highest priority is Audio Thread. The audio plugin typically runs on two threads because the UI and the audio engine need to be completely separate. The audio engine must never sacrifice its processing for the UI. In fact, it shouldn't even be aware that the UI exists. There are many good practices to follow when dealing with multi-threaded audio based applications, but the main goal is our audio engine should never be blocked from running fast and efficiently. This means that we must abide by and be sure to follow on our audio thread.

[Ross Bencina](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)

Ross Bencia's rules of thumb or real-time audio callback programming:

- Don't allocate or de-allocate memory.
- Don't lock a mutex.
- Don't read or write to the file system or otherwise perform I/O.
- Don't execute any code that has unpredictable or poor worst-case timing behavior.
- Don't call any code that may violate the rules established above.

---

## JUCE: Parameters, Components & Listeners

We can create an Audio Plugin through Projucer, and open the project in Xcode. We can choose the active scheme (VST, Audio Unit etc), and select an executable in the 'Run' pane of the Scheme Editor. Choosing Ableton Live will execute Live once Xcode finishes building the application. We can then drag that plugin into a blank track to see it run Live! Note that in High Sierra there is a known issue for detecting these plugins. A quick restart should fix it!

![](img/-0c90ec71-eee0-44f3-828c-7956e8eec75cuntitled)

Evaluating the directory structure of the source code, we see:

    PluginUno/Source
    ├── PluginEditor.cpp
    ├── PluginEditor.h
    ├── PluginProcessor.cpp
    └── PluginProcessor.h

Here, we see two files (.h and .cpp combined) representing the two threads we discussed earlier. `PluginEditor` is the GUI, `PluginProcessor` is the audio engine. JUCE handles the creation of both of these classes. To disable the GUI, we can return false on these two implementations:

    bool PluginUnoAudioProcessor::hasEditor() const
    {
        return false; // (change this to false if you choose to not supply an editor)
    }

    AudioProcessorEditor* PluginUnoAudioProcessor::createEditor()
    {
        // return new PluginUnoAudioProcessorEditor (*this);
    		return nullptr;
    }

Our application does not need a UI to function. This relates back to the necessary independence of the application's UI and audio thread. When the UI is closed, it's fully deleted. This means if the audio thread is referencing the UI, it will crash the application. It's best to focus on the `PluginProcessor` for now.

### PluginProcessor: `processBlock()`

We see that our `PluginProcessor` inherits from the base class `AudioProcessor:`

    class PluginUnoAudioProcessor  : public AudioProcessor
    {
    	// ... header definitions omitted
    }

    // In juce_AudioProcessor.h

    //==============================================================================
    /**
        Base class for audio processing classes or plugins.

        This is intended to act as a base class of audio processor that is general enough
        to be wrapped as a VST, AU, RTAS, etc, or used internally.

        It is also used by the plugin hosting code as the wrapper around an instance
        of a loaded plugin.

        You should derive your own class from this base class, and if you're building a
        plugin, you should implement a global function called createPluginFilter() which
        creates and returns a new instance of your subclass.

        @tags{Audio}
    */
    class JUCE_API  AudioProcessor {}

The `AudioProcessor` is the base class we should use as a jumping point for our plugin. The most important functions for audio processing are `processBlock` and `prepareToPlay`.

`void prepareToPlay(double sampleRate, int samplesPerBlock)` is a function that gets called for pre-playback initialization. It's likely that we'll change our setup to account for buffer size and sample rates in the host DAW.

`void processBlock(AudioBuffer<float>&, MidiBuffer&)` is where the true audio processing happens. This is where we'll write most of the DSP functions.

Since we're focusing on creating a basic audio effect and not a synthesizer, we're going to focus on the `AudioBuffer` JUCE class. The `AudioBuffer` is the audio coming in from the DAW, it's our responsibility to quickly process and return this data to our host by finishing our function in time. The AudioBuffer is a discrete signal stored as floating point values inside the computer. Thus, `AudioBuffer` class is simply a wrapper around an array of floating point values which represent the audio signal, with each point representing the position of the waveform in time. How may we manipulate the volume of the signal?

    // Inside processBlock...
    // Iterate through the number of channels, (typically 2)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer (channel);

    		// Run through the buffer and multiply each sample by half to halve the volume
        for (int sample = 0; sample < buffer.getNumSamples(); sample++)
        {
            channelData[sample] *= 0.5;
        }
    }

Running this will simply reduce the volume of the audio signal by half, which is hardly useful since it's hardcoded. It's better to make it a gain knob that can be controlled by the user.

To create a native audio parameter, we declare in our processor's header `AudioParameterFloat* mGainParam;` which notifies the host that this is a controllable parameter. We could make a float scoped to the processBlock routine, but that wouldn't expose the parameter to the user for automation. `AudioParameterFloat` is a way of declaring a parameter that makes the host aware of the parameter! We must now construct the parameter and register it inside of the application constructor like so:

    PluginUnoAudioProcessor::PluginUnoAudioProcessor()
    {
    		addParameter(mGainParam = new AudioParameterFloat("gain", "Gain", 0.0f, 1.0f, 0.5f));
    }

Now that we've added and initialized the parameter, we can access it inside of our `processBlock`. So instead of multiplying each sample of the buffer by 0.5, we multiply it by the gain parameter we just created:

    channelData[sample] *= mGainParam->get();

Now we can see that it registered inside the host! We've got a controllable value. Thanks to the JUCE API, we've got a complex interaction with the host wrapped into an abstracted `AudioParameter` class. We can now adjust the gain knob supplied the host without any sort of GUI. Hooray.

![](img/-9277555e-794a-464b-8e78-80e06de4c219untitled)

Another common issue when working with digital audio is discontinuities. Discontinuities occur when a change in the signal is so great that it creates a step (and a click) in the waveform. Currently with the gain knob, we can hear slight crackles and pops in the output as we change the value quickly. This is because the gain parameter is being immediately set to the parameter value at whatever rate the parameter is being edited. We always want our parameter value to be snappy and responsive, but it isn't necessarily what we want to apply direct to the audio signal. What we really want to do is set our gain parameter as a target volume, and smoothly travel to the value in a relatively short amount of time (enough to be imperceptible). This is called signal smoothing and is a very important concept in digital audio. We'll use a new audio processor variable `mGainSmooth` to smoothly change our audio signal's volume to the target gain parameter:

    // x = smoothedvalue, y = targetValue, z = scalar (speed)
    // x = x - z * (x - y)

    gainSmoothed = gainSmoothed - scalar * (gainSmoothed - targetGain)

We'll also avoid iterating through the two channels since we want to change the signal simulatenously. Removing the outer for loop and accessing the stereo channels directly, we can modify their signal at the same time. We'll also use a small scalar to ensure the smoothing is granular. In action, that looks like:

    float* channelLeft = buffer.getWritePointer(0);
    float* channelRight = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); sample++)
    {
        mGainSmoothed = mGainSmoothed - 0.004 * (mGainSmoothed - mGainParam->get());

        channelLeft[sample] *= mGainSmoothed;
        channelRight[sample] *= mGainSmoothed;
    }

### Adding UI

The core of the JUCE GUI framework relies on the `Component` class. Evaluate the `juce_Component.h` header to see all functionality provided. Everything on the screen inherits from this class, including the `PluginEditor`, the GUI of our plugin. The JUCE SDK folder has tons of examples, let's run the `DemoRunner` to examine all the included components.

In order to add a slider to our GUI, we can declare a member variable `mGainControlSlider` of data type `Slider` and initialize in our Editor class's constructor.

    // In PluginEditor.h
    Slider mGainControlSlider;

    // In PluginEditor.cpp constructor after setting window size
    mGainControlSlider.setBounds(0, 0, 100, 100);
    mGainControlSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    mGainControlSlider.setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(mGainControlSlider);

### Connecting a GUI component to an AudioParameter

JUCE's listener pattern is quite similar to the popular _Observer Pattern_. It addresses the following problems:

- A one-to-many dependency between many objects should be defined without making the objects tightly coupled.
- It should be ensured that when one object changes state, an open-ended number of dependent objects are updated automatically.
- It should be possible that one object can notify an open-ended number of other objects.

We'll inherit from the JUCE `Listener` abstract class when defining our Component classes. Since C++ allows multiple inheritance, we don't have to resort to interfaces, but the functionality is similar. For example, the below definition will force us to override the `sliderDidChange` virtual function in our implementation file. We'll then register the actual listener in the editor's constructor `mGainControlSlider.addListener(this);`

    // In header
    class PluginUnoAudioProcessorEditor  : public AudioProcessorEditor, public Slider::Listener
    {
    public:
    		// ... Stuff!
    		// ...
    		void sliderValueChanged (Slider* slider) override;
    }

    // In implementation file
    void PluginUnoAudioProcessorEditor::sliderValueChanged(Slider *slider)
    {
        if (slider == &mGainControlSlider) {
            DBG("Slider value changed.");
        }
    }

All is well, and our `sliderDidChange` callback is firing and printing the debug logs!

In order to access the actual gain `AudioParameter` inside the Audio Processor, we'll need to define a pointer for it and set its value like so:

    // Cast the base AudioParameter class to a float from the 0th index of the parameter array
    AudioParameterFloat* gainParameter = (AudioParameterFloat*)params.getUnchecked(0);
    *gainParameter = mGainControlSlider.getValue();

We also need to set some more intial state for the slide in our Editor constructor. NOTE: You should avoid printing from the Process block, but it's fine for debugging.

    auto& params = processor.getParameters();
    AudioParameterFloat* gainParam = (AudioParameterFloat*)params.getUnchecked(0);
    mGainControlSlider.setRange(gainParam->range.start, gainParam->range.end);

We can also assign lambdas instead of registering listeners, which can be more flexible. So it's okay to remove the `Listener` subclass from the Editor's definition file and instead assign a lambda in the constructor of the Editor like so:

    mGainControlSlider.onValueChange = [this, gainParam] {
        *gainParam = mGainControlSlider.getValue();
    };

    // Implement the other listeners for robustness
    // This is going to ensure that this funciton works well in most DAWs
    mGainControlSlider.onDragStart = [gainParam] {
        gainParam->beginChangeGesture();
    };

    mGainControlSlider.onDragEnd = [gainParam] {
        (*gainParam).endChangeGesture();
    };

---

## Delays, Circular Buffers & Interpolation

> Delay is an audio effect and an effects unit which records an input signal to an audio storage medium, and then plays it back after a period of time. The delayed signal may either be played back multiple times, or played back into the recording again, to create the sound of a repeating, decaying echo.

In the analog world, this is a tricky circuit, which requires a series of capacitors charging and emptying to delay the signal. In the digital world, we're lucky that this is much simpler. Let's examine a DSP block diagram for delay:

![](img/-ea056293-4a9a-4cec-b3a0-b2615a67890cuntitled)

`x(t)` is the input, while `y(t)` is the output. Inside of the block, `x(t)` is sent to 2 places: a block which delays the signal by time `t`, this is the audio storage, and a summation stage where the input signal and delayed signal are summed. This is a rudimentary delay that lacks any feedback or filtering, but is the simplest startest point.

For storing our audio signal, we'll hold it in a data structure known as circular buffer, which will continually be filled with audio data. When we run out of space in the buffer, our delayed signal should have already been played back, thus making it safe to overwrite it with new data. This type of buffer is heavily used in digital audio systems. This single, fixed-size buffer is connected end-to-end, which lends itself nicely to buffering data streams.

### Circular Buffer

We'll create an array of floats to hold our input audio signal. The length of the buffer needs to be long enough to handle our longest desired delay time. How do we determine the amount of floats that is?

If our sample rate is 44.1kHz, theres 44.1k samples of audio streaming a second. If we want 2 seconds to be our max delay time, the equation for our delay time length is:

    CircularBufferSize = sampleRate * maxDelayTime
