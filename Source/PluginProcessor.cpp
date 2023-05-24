#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Ultralight/Renderer.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
        : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
), parameters (*this, nullptr, "PARAMETERS", {
        std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f, 1.0f, 0.5f)
})
{
    // Add parameters to the AudioProcessorValueTreeState
    parameters.state = juce::ValueTree(juce::Identifier("MyAudioProcessor"));
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
//    renderer->PurgeMemory();
//    renderer = nullptr;
}

void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    if (! juce::Process::isForegroundProcess())
        juce::Process::makeForegroundProcess();

    DBG(sampleRate);
    DBG(samplesPerBlock);
}


void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    buffer.clear();

    // Get the current value of the "gain" parameter
    float gain = *parameters.getRawParameterValue("gain");
    // Apply the gain to the audio buffer
    buffer.applyGain(gain);

    juce::ScopedNoDenormals noDenormals;
}













//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    auto editor = new PluginAudioProcessorEditor (*this);

    // Check if standalone app
    if (auto* app = dynamic_cast<juce::JUCEApplicationBase*> (juce::JUCEApplication::getInstance()))
        if (app->isStandaloneApp()){
//            if(juce::TopLevelWindow::getNumTopLevelWindows() > 1)
//            {
            juce::TopLevelWindow* w = juce::TopLevelWindow::getTopLevelWindow(0);
            w->setUsingNativeTitleBar(true);
//            }
        }
    return editor;
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save the APVTS state to persist parameter values
    juce::MemoryOutputStream stream(destData, false);
    parameters.state.writeToStream(stream);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Load the APVTS state to restore parameter values
    juce::MemoryInputStream stream(data, sizeInBytes, false);
    parameters.state = juce::ValueTree::readFromStream(stream);
}

//==============================================================================

// HYPERIMPORTANT: Since Ultralight has a hard constraint on the thread that first creates and then calls the
// renderer, we store it globally (this is shared between all instances of the plugin)
// The docs say to have one renderer per application
// In DAW-land, application == DAW, application != plugin -> we have one renderer per DAW!!!!!!!
// It follows that we have to use this particular instance and NEVER create a new one
// Doing so will invalidate the first one and cause a crash
ultralight::RefPtr<ultralight::Renderer> AudioPluginAudioProcessor::RENDERER = nullptr;

// This creates new instances of the plugin
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    // ================================== ULTRALIGHT ==================================
    Config config;

    // We need to tell config where our resources are so it can load our bundled SSL certificates to make HTTPS requests.
    config.resource_path = "/Users/max/CLionProjects/ultralight-juce/Libs/ultralight-sdk/bin/resources";
    // The GPU renderer should be disabled to render Views to a pixel-buffer (Surface).
    config.use_gpu_renderer = false;
    // You can set a custom DPI scale here. Default is 1.0 (100%)
    auto scale = juce::Desktop::getInstance().getDisplays().displays[0].scale;
    config.device_scale = scale;

    // Pass our configuration to the Platform singleton so that the library can use it.
    Platform::instance().set_config(config);
    // Use the OS's native font loader
    Platform::instance().set_font_loader(GetPlatformFontLoader());
    // Use the OS's native file loader, with a base directory
    // All file:// URLs will load relative to this base directory.
    // For shipping, this needs to be tied to JUCE Binary files or a custom resource directory
    Platform::instance().set_file_system(GetPlatformFileSystem("/Users/max/CLionProjects/ultralight-juce/Resources"));
    // Use the default logger (writes to a log file)
    Platform::instance().set_logger(GetDefaultLogger("ultralight.log"));

    // This makes sure we only have ONE renderer per application
    if(AudioPluginAudioProcessor::RENDERER.get() == nullptr)
        AudioPluginAudioProcessor::RENDERER = Renderer::Create();

    return new AudioPluginAudioProcessor();
}
