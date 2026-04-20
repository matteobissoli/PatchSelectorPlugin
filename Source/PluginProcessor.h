#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "model/PatchLibrary.h"
#include <array>
#include <optional>

class PatchSelectorAudioProcessorEditor;

class PatchSelectorAudioProcessor : public juce::AudioProcessor
{
public:
    PatchSelectorAudioProcessor();
    ~PatchSelectorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool loadPatchLibrary(const juce::File& file, juce::String& error);
    bool loadAvailableLibraryByIndex(int index, juce::String& error);

    const PatchLibrary* getPatchLibrary() const noexcept { return patchLibrary ? &(*patchLibrary) : nullptr; }
    juce::String getLoadedLibraryPath() const { return loadedLibraryPath; }
    juce::String getLastErrorMessage() const { return lastErrorMessage; }

    juce::StringArray getCategories() const;
    std::vector<int> buildVisiblePatchIndices(const juce::String& categoryFilter, const juce::String& searchText) const;

    bool selectPatchByRealIndex(int realPatchIndex, bool autoSend);
    int getSelectedPatchIndex() const noexcept { return selectedPatchIndex; }

    int getMidiChannel() const;
    void setMidiChannel(int channel);

    bool getAutoSendOnSelection() const;
    void setAutoSendOnSelection(bool shouldAutoSend);

    bool getSendOnLoad() const;
    void setSendOnLoad(bool shouldSendOnLoad);

    bool getResendOnTransportStart() const;
    void setResendOnTransportStart(bool shouldResend);

    void sendSelectedPatchNow();
    int getTestMidiChannel() const;
    void setTestMidiChannel(int channel);
    int getSoundTestChordIndex() const noexcept;
    void setSoundTestChordIndex(int index) noexcept;
    void startSoundTest();
    void stopSoundTest();

    juce::File getLibraryFolder() const;
    void ensureLibraryFolderExists() const;
    void refreshAvailableLibraries();
    juce::StringArray getAvailableLibraryNames() const;
    int getLoadedLibraryIndex() const;
    bool revealLibraryFolder() const;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }

private:
    juce::AudioProcessorValueTreeState apvts;
    std::optional<PatchLibrary> patchLibrary;

    juce::Array<juce::MidiMessage> pendingMidiMessages;
    juce::CriticalSection pendingMidiLock;

    juce::Array<juce::File> availableLibraryFiles;
    juce::StringArray availableLibraryNames;

    int selectedPatchIndex = -1;
    juce::String loadedLibraryPath;
    juce::String lastErrorMessage;
    bool lastTransportPlaying = false;
    int detectedMidiChannel = 1;
    bool soundTestActive = false;
    int soundTestChannel = 1;
    int soundTestChordIndex = 0;
    bool hasInitialisedTestMidiChannel = false;
    std::array<int, 3> activeSoundTestNotes { 60, 64, 67 };

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void enqueueMidiMessages(const juce::Array<juce::MidiMessage>& messages);
    juce::Array<juce::MidiMessage> createSelectedPatchMessages(int midiChannel) const;
    int getDetectedMidiChannel() const noexcept;
    bool isTransportCurrentlyPlaying() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchSelectorAudioProcessor)
};
