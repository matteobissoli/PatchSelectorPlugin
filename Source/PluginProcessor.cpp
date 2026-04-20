#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "repository/PatchLibraryLoader.h"
#include "midi/MidiDispatchEngine.h"
#include <algorithm>

namespace IDs
{
    static constexpr auto midiChannel = "midiChannel";
    static constexpr auto autoSendOnSelection = "autoSendOnSelection";
    static constexpr auto sendOnLoad = "sendOnLoad";
    static constexpr auto resendOnTransportStart = "resendOnTransportStart";
}

PatchSelectorAudioProcessor::PatchSelectorAudioProcessor()
    : AudioProcessor(BusesProperties()),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    ensureLibraryFolderExists();
    refreshAvailableLibraries();

    if (availableLibraryFiles.size() > 0)
    {
        juce::String error;
        loadAvailableLibraryByIndex(0, error);
    }
}

void PatchSelectorAudioProcessor::prepareToPlay(double, int)
{
    lastTransportPlaying = false;
    detectedMidiChannel = getTestMidiChannel();
}

void PatchSelectorAudioProcessor::releaseResources()
{
    stopSoundTest();
}

bool PatchSelectorAudioProcessor::isBusesLayoutSupported(const BusesLayout&) const
{
    return true;
}

void PatchSelectorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.getChannel() >= 1 && message.getChannel() <= 16)
            detectedMidiChannel = message.getChannel();
    }

    juce::Array<juce::MidiMessage> toSend;
    {
        const juce::ScopedLock lock(pendingMidiLock);
        toSend.swapWith(pendingMidiMessages);
    }

    const auto isPlayingNow = isTransportCurrentlyPlaying();
    if (getResendOnTransportStart() && isPlayingNow && ! lastTransportPlaying)
        toSend.addArray(createSelectedPatchMessages(getDetectedMidiChannel()));

    lastTransportPlaying = isPlayingNow;

    int sampleOffset = 0;
    for (const auto& message : toSend)
        midiMessages.addEvent(message, sampleOffset++);
}

juce::AudioProcessorEditor* PatchSelectorAudioProcessor::createEditor()
{
    return new PatchSelectorAudioProcessorEditor(*this);
}

void PatchSelectorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("libraryPath", loadedLibraryPath, nullptr);
    state.setProperty("selectedPatchIndex", selectedPatchIndex, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PatchSelectorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    auto state = juce::ValueTree::fromXml(*xmlState);
    if (! state.isValid())
        return;

    apvts.replaceState(state);
    hasInitialisedTestMidiChannel = true;

    refreshAvailableLibraries();

    loadedLibraryPath = state.getProperty("libraryPath").toString();
    selectedPatchIndex = static_cast<int>(state.getProperty("selectedPatchIndex", -1));

    if (loadedLibraryPath.isNotEmpty())
    {
        juce::String error;
        if (loadPatchLibrary(juce::File(loadedLibraryPath), error) && getSendOnLoad())
            sendSelectedPatchNow();
    }
    else if (availableLibraryFiles.size() > 0)
    {
        juce::String error;
        loadAvailableLibraryByIndex(0, error);
    }
}

bool PatchSelectorAudioProcessor::loadPatchLibrary(const juce::File& file, juce::String& error)
{
    auto loaded = PatchLibraryLoader::loadFromFile(file, error);
    if (! loaded.has_value())
    {
        lastErrorMessage = error;
        return false;
    }

    patchLibrary = std::move(loaded);
    loadedLibraryPath = file.getFullPathName();
    lastErrorMessage.clear();

    if (selectedPatchIndex >= (int) patchLibrary->patches.size())
        selectedPatchIndex = -1;

    if (selectedPatchIndex < 0 && ! patchLibrary->patches.empty())
        selectedPatchIndex = 0;

    if (! hasInitialisedTestMidiChannel
        && patchLibrary->device.defaultMidiChannel >= 1
        && patchLibrary->device.defaultMidiChannel <= 16)
        setTestMidiChannel(patchLibrary->device.defaultMidiChannel);

    return true;
}

bool PatchSelectorAudioProcessor::loadAvailableLibraryByIndex(int index, juce::String& error)
{
    refreshAvailableLibraries();

    if (index < 0 || index >= availableLibraryFiles.size())
    {
        error = "Invalid library selection.";
        lastErrorMessage = error;
        return false;
    }

    return loadPatchLibrary(availableLibraryFiles[index], error);
}

juce::StringArray PatchSelectorAudioProcessor::getCategories() const
{
    juce::StringArray categories;
    categories.add("All");

    if (! patchLibrary.has_value())
        return categories;

    for (const auto& patch : patchLibrary->patches)
        if (patch.category.isNotEmpty() && ! categories.contains(patch.category))
            categories.add(patch.category);

    categories.removeString("All");
    categories.sort(true);
    categories.insert(0, "All");
    return categories;
}

std::vector<int> PatchSelectorAudioProcessor::buildVisiblePatchIndices(const juce::String& categoryFilter, const juce::String& searchText) const
{
    std::vector<int> indices;

    if (! patchLibrary.has_value())
        return indices;

    const auto search = searchText.trim().toLowerCase();
    const auto category = categoryFilter.trim();

    for (int i = 0; i < (int) patchLibrary->patches.size(); ++i)
    {
        const auto& patch = patchLibrary->patches[(size_t) i];

        const bool categoryMatch = category.isEmpty() || category == "All" || patch.category == category;
        const auto haystack = (patch.name + " " + patch.category + " " + patch.subcategory + " " + patch.tags.joinIntoString(" ")).toLowerCase();
        const bool searchMatch = search.isEmpty() || haystack.contains(search);

        if (categoryMatch && searchMatch)
            indices.push_back(i);
    }

    return indices;
}

bool PatchSelectorAudioProcessor::selectPatchByRealIndex(int realPatchIndex, bool autoSend)
{
    if (! patchLibrary.has_value())
        return false;

    if (realPatchIndex < 0 || realPatchIndex >= (int) patchLibrary->patches.size())
        return false;

    selectedPatchIndex = realPatchIndex;

    if (autoSend)
        sendSelectedPatchNow();

    return true;
}

int PatchSelectorAudioProcessor::getMidiChannel() const
{
    return (int) apvts.getRawParameterValue(IDs::midiChannel)->load();
}

void PatchSelectorAudioProcessor::setMidiChannel(int channel)
{
    setTestMidiChannel(channel);
}

int PatchSelectorAudioProcessor::getTestMidiChannel() const
{
    return (int) apvts.getRawParameterValue(IDs::midiChannel)->load();
}

void PatchSelectorAudioProcessor::setTestMidiChannel(int channel)
{
    auto* parameter = apvts.getParameter(IDs::midiChannel);
    jassert(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1((float) juce::jlimit(1, 16, channel)));
    hasInitialisedTestMidiChannel = true;
}

int PatchSelectorAudioProcessor::getSoundTestChordIndex() const noexcept
{
    return soundTestChordIndex;
}

void PatchSelectorAudioProcessor::setSoundTestChordIndex(int index) noexcept
{
    soundTestChordIndex = juce::jlimit(0, 23, index);
}

bool PatchSelectorAudioProcessor::getAutoSendOnSelection() const
{
    return apvts.getRawParameterValue(IDs::autoSendOnSelection)->load() > 0.5f;
}

void PatchSelectorAudioProcessor::setAutoSendOnSelection(bool shouldAutoSend)
{
    auto* parameter = apvts.getParameter(IDs::autoSendOnSelection);
    jassert(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(shouldAutoSend ? 1.0f : 0.0f));
}

bool PatchSelectorAudioProcessor::getSendOnLoad() const
{
    return apvts.getRawParameterValue(IDs::sendOnLoad)->load() > 0.5f;
}

void PatchSelectorAudioProcessor::setSendOnLoad(bool shouldSendOnLoad)
{
    auto* parameter = apvts.getParameter(IDs::sendOnLoad);
    jassert(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(shouldSendOnLoad ? 1.0f : 0.0f));
}

bool PatchSelectorAudioProcessor::getResendOnTransportStart() const
{
    return apvts.getRawParameterValue(IDs::resendOnTransportStart)->load() > 0.5f;
}

void PatchSelectorAudioProcessor::setResendOnTransportStart(bool shouldResend)
{
    auto* parameter = apvts.getParameter(IDs::resendOnTransportStart);
    jassert(parameter != nullptr);
    parameter->setValueNotifyingHost(parameter->convertTo0to1(shouldResend ? 1.0f : 0.0f));
}

void PatchSelectorAudioProcessor::sendSelectedPatchNow()
{
    enqueueMidiMessages(createSelectedPatchMessages(getTestMidiChannel()));
}

void PatchSelectorAudioProcessor::startSoundTest()
{
    if (soundTestActive)
        return;

    static constexpr std::array<int, 12> rootNotes { 60, 67, 62, 69, 64, 71, 66, 61, 68, 63, 70, 65 };

    const auto rootIndex = soundTestChordIndex / 2;
    const auto isMinor = (soundTestChordIndex % 2) != 0;
    soundTestChannel = getTestMidiChannel();
    soundTestActive = true;
    activeSoundTestNotes = { rootNotes[(size_t) rootIndex],
                             rootNotes[(size_t) rootIndex] + (isMinor ? 3 : 4),
                             rootNotes[(size_t) rootIndex] + 7 };

    juce::Array<juce::MidiMessage> messages;
    messages.add(juce::MidiMessage::noteOn(soundTestChannel, activeSoundTestNotes[0], (juce::uint8) 100));
    messages.add(juce::MidiMessage::noteOn(soundTestChannel, activeSoundTestNotes[1], (juce::uint8) 100));
    messages.add(juce::MidiMessage::noteOn(soundTestChannel, activeSoundTestNotes[2], (juce::uint8) 100));
    enqueueMidiMessages(messages);
}

void PatchSelectorAudioProcessor::stopSoundTest()
{
    if (! soundTestActive)
        return;

    juce::Array<juce::MidiMessage> messages;
    messages.add(juce::MidiMessage::noteOff(soundTestChannel, activeSoundTestNotes[0]));
    messages.add(juce::MidiMessage::noteOff(soundTestChannel, activeSoundTestNotes[1]));
    messages.add(juce::MidiMessage::noteOff(soundTestChannel, activeSoundTestNotes[2]));
    enqueueMidiMessages(messages);

    soundTestActive = false;
}

juce::File PatchSelectorAudioProcessor::getLibraryFolder() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("PatchSelectorPlugin")
        .getChildFile("Libraries");
}

void PatchSelectorAudioProcessor::ensureLibraryFolderExists() const
{
    auto folder = getLibraryFolder();
    if (! folder.exists())
        folder.createDirectory();
}

void PatchSelectorAudioProcessor::refreshAvailableLibraries()
{
    ensureLibraryFolderExists();

    availableLibraryFiles.clear();
    availableLibraryNames.clear();
    const auto folder = getLibraryFolder();
    auto files = folder.findChildFiles(juce::File::findFiles, false, "*.json");
    std::sort(files.begin(), files.end(), [](const juce::File& a, const juce::File& b)
    {
        return a.getFileNameWithoutExtension().compareNatural(b.getFileNameWithoutExtension()) < 0;
    });

    for (const auto& file : files)
    {
        availableLibraryFiles.add(file);

        juce::String error;
        auto loadedLibrary = PatchLibraryLoader::loadFromFile(file, error);
        const auto displayName = loadedLibrary.has_value() ? loadedLibrary->getDisplayName()
                                                           : file.getFileNameWithoutExtension();
        availableLibraryNames.add(displayName);
    }
}

juce::StringArray PatchSelectorAudioProcessor::getAvailableLibraryNames() const
{
    return availableLibraryNames;
}

int PatchSelectorAudioProcessor::getLoadedLibraryIndex() const
{
    for (int i = 0; i < availableLibraryFiles.size(); ++i)
        if (availableLibraryFiles.getReference(i).getFullPathName() == loadedLibraryPath)
            return i;

    return -1;
}

bool PatchSelectorAudioProcessor::revealLibraryFolder() const
{
    ensureLibraryFolderExists();
    getLibraryFolder().revealToUser();
    return true;
}

juce::AudioProcessorValueTreeState::ParameterLayout PatchSelectorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterInt>(IDs::midiChannel,
                                                                   "MIDI Channel",
                                                                   1,
                                                                   16,
                                                                   1));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(IDs::autoSendOnSelection,
                                                                    "Auto Send On Selection",
                                                                    true));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(IDs::sendOnLoad,
                                                                    "Send On Load",
                                                                    true));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(IDs::resendOnTransportStart,
                                                                    "Resend On Transport Start",
                                                                    true));

    return { parameters.begin(), parameters.end() };
}

void PatchSelectorAudioProcessor::enqueueMidiMessages(const juce::Array<juce::MidiMessage>& messages)
{
    if (messages.isEmpty())
        return;

    const juce::ScopedLock lock(pendingMidiLock);
    pendingMidiMessages.addArray(messages);
}

juce::Array<juce::MidiMessage> PatchSelectorAudioProcessor::createSelectedPatchMessages(int midiChannel) const
{
    if (! patchLibrary.has_value())
        return {};

    if (selectedPatchIndex < 0 || selectedPatchIndex >= (int) patchLibrary->patches.size())
        return {};

    const auto& patch = patchLibrary->patches[(size_t) selectedPatchIndex];
    return MidiDispatchEngine::createPatchSelectMessages(patch,
                                                         midiChannel,
                                                         patchLibrary->device.programBase);
}

int PatchSelectorAudioProcessor::getDetectedMidiChannel() const noexcept
{
    return juce::jlimit(1, 16, detectedMidiChannel);
}

bool PatchSelectorAudioProcessor::isTransportCurrentlyPlaying() const
{
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
            return position->getIsPlaying();
    }

    return false;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PatchSelectorAudioProcessor();
}
