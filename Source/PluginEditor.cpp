#include "PluginEditor.h"
#include <algorithm>

namespace
{
constexpr int margin = 10;
constexpr std::array<const char*, 24> soundTestChordNames
{
    "C", "Cm",
    "G", "Gm",
    "D", "Dm",
    "A", "Am",
    "E", "Em",
    "B", "Bm",
    "F#", "F#m",
    "Db", "Dbm",
    "Ab", "Abm",
    "Eb", "Ebm",
    "Bb", "Bbm",
    "F", "Fm"
};
}

PatchSelectorAudioProcessorEditor::PatchSelectorAudioProcessorEditor(PatchSelectorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(900, 500);

    libraryLabel.setText("Library", juce::dontSendNotification);
    categoryLabel.setText("Category", juce::dontSendNotification);
    searchLabel.setText("Search", juce::dontSendNotification);
    midiChannelLabel.setText("MIDI Ch.", juce::dontSendNotification);
    selectedPatchHeaderLabel.setText("Patch", juce::dontSendNotification);
    rescanButton.setButtonText("Rescan");
    rescanButton.setTooltip("Refresh library list");
    openFolderButton.setButtonText("Folder");
    openFolderButton.setTooltip("Open library folder");
    autoSendToggle.setButtonText("Auto Send");
    sendOnLoadToggle.setButtonText("Send on Load");
    resendOnTransportToggle.setButtonText("Transport Start");

    searchEditor.setTextToShowWhenEmpty("Type to filter patches...", juce::Colours::grey);
    searchEditor.addListener(this);

    libraryCombo.addListener(this);
    categoryCombo.addListener(this);
    soundTestChordCombo.addListener(this);

    for (int ch = 1; ch <= 16; ++ch)
        midiChannelCombo.addItem(juce::String(ch), ch);

    for (int i = 0; i < (int) soundTestChordNames.size(); ++i)
        soundTestChordCombo.addItem(soundTestChordNames[(size_t) i], i + 1);

    soundTestChordCombo.setSelectedItemIndex(audioProcessor.getSoundTestChordIndex(), juce::dontSendNotification);

    patchList.setRowHeight(28);
    patchList.setColour(juce::ListBox::backgroundColourId, juce::Colours::black.withAlpha(0.2f));
    selectedPatchHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    selectedPatchValuesLabel.setJustificationType(juce::Justification::centredRight);
    selectedPatchNameLabel.setJustificationType(juce::Justification::centredLeft);
    selectedPatchHeaderLabel.setFont(juce::FontOptions(11.0f));
    selectedPatchValuesLabel.setFont(juce::FontOptions(11.0f));
    selectedPatchNameLabel.setFont(juce::FontOptions(20.0f, juce::Font::bold));

    openFolderButton.addListener(this);
    rescanButton.addListener(this);
    sendButton.addListener(this);
    soundTestButton.addListener(this);
    previousPatchButton.addListener(this);
    nextPatchButton.addListener(this);

    addAndMakeVisible(deviceLabel);
    addAndMakeVisible(statusLabel);
    addAndMakeVisible(libraryLabel);
    addAndMakeVisible(libraryCombo);
    addAndMakeVisible(categoryLabel);
    addAndMakeVisible(categoryCombo);
    addAndMakeVisible(searchLabel);
    addAndMakeVisible(searchEditor);
    addAndMakeVisible(midiChannelLabel);
    addAndMakeVisible(midiChannelCombo);
    addAndMakeVisible(soundTestChordCombo);
    addAndMakeVisible(selectedPatchHeaderLabel);
    addAndMakeVisible(selectedPatchValuesLabel);
    addAndMakeVisible(selectedPatchNameLabel);
    addAndMakeVisible(openFolderButton);
    addAndMakeVisible(rescanButton);
    addAndMakeVisible(sendButton);
    addAndMakeVisible(soundTestButton);
    addAndMakeVisible(previousPatchButton);
    addAndMakeVisible(nextPatchButton);
    addAndMakeVisible(autoSendToggle);
    addAndMakeVisible(sendOnLoadToggle);
    addAndMakeVisible(resendOnTransportToggle);
    addAndMakeVisible(patchList);

    autoSendAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getValueTreeState(),
                                                                                                 "autoSendOnSelection",
                                                                                                 autoSendToggle);
    sendOnLoadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getValueTreeState(),
                                                                                                   "sendOnLoad",
                                                                                                   sendOnLoadToggle);
    resendOnTransportAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.getValueTreeState(),
                                                                                                          "resendOnTransportStart",
                                                                                                          resendOnTransportToggle);
    midiChannelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.getValueTreeState(),
                                                                                                      "midiChannel",
                                                                                                      midiChannelCombo);

    refreshView();
    startTimerHz(10);
}

PatchSelectorAudioProcessorEditor::~PatchSelectorAudioProcessorEditor()
{
    openFolderButton.removeListener(this);
    rescanButton.removeListener(this);
    sendButton.removeListener(this);
    soundTestButton.removeListener(this);
    previousPatchButton.removeListener(this);
    nextPatchButton.removeListener(this);
    libraryCombo.removeListener(this);
    categoryCombo.removeListener(this);
    soundTestChordCombo.removeListener(this);
    searchEditor.removeListener(this);
}

void PatchSelectorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(30, 30, 34));

    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4.0f), 6.0f, 1.0f);
}

void PatchSelectorAudioProcessorEditor::resized()
{
    constexpr int controlGap = 10;
    constexpr int labelHeight = 18;
    constexpr int topBarHeight = 30;
    constexpr int controlHeight = 28;
    constexpr int sectionGap = 8;
    constexpr int toggleRowHeight = 26;
    constexpr int leftColumnWidth = 260;
    constexpr int refreshButtonWidth = 78;
    constexpr int folderButtonWidth = 72;

    auto area = getLocalBounds().reduced(margin);

    auto topBar = area.removeFromTop(topBarHeight);
    openFolderButton.setBounds(topBar.removeFromLeft(folderButtonWidth).withHeight(topBarHeight));
    topBar.removeFromLeft(controlGap);
    rescanButton.setBounds(topBar.removeFromRight(refreshButtonWidth).withHeight(topBarHeight));
    topBar.removeFromRight(controlGap);
    libraryCombo.setBounds(topBar.withHeight(topBarHeight));

    area.removeFromTop(sectionGap);

    auto leftColumn = area.removeFromLeft(leftColumnWidth);
    area.removeFromLeft(sectionGap);
    auto rightColumn = area;

    deviceLabel.setBounds({});

    if (statusLabel.getText().isNotEmpty())
    {
        leftColumn.removeFromTop(2);
        statusLabel.setBounds(leftColumn.removeFromTop(18));
        leftColumn.removeFromTop(sectionGap);
    }
    else
    {
        statusLabel.setBounds({});
    }

    categoryLabel.setBounds(leftColumn.removeFromTop(labelHeight));
    categoryCombo.setBounds(leftColumn.removeFromTop(controlHeight));
    leftColumn.removeFromTop(sectionGap);

    searchLabel.setBounds(leftColumn.removeFromTop(labelHeight));
    searchEditor.setBounds(leftColumn.removeFromTop(controlHeight));
    leftColumn.removeFromTop(sectionGap);

    autoSendToggle.setBounds(leftColumn.removeFromTop(toggleRowHeight));
    leftColumn.removeFromTop(4);
    sendOnLoadToggle.setBounds(leftColumn.removeFromTop(toggleRowHeight));
    leftColumn.removeFromTop(4);
    resendOnTransportToggle.setBounds(leftColumn.removeFromTop(toggleRowHeight));
    leftColumn.removeFromTop(sectionGap);

    midiChannelLabel.setBounds(leftColumn.removeFromTop(labelHeight));
    auto midiRow = leftColumn.removeFromTop(controlHeight);
    auto midiArea = midiRow.removeFromLeft(88);
    midiChannelCombo.setBounds(midiArea);
    midiRow.removeFromLeft(controlGap);
    sendButton.setBounds(midiRow.withHeight(controlHeight));

    leftColumn.removeFromTop(sectionGap);
    auto soundTestRow = leftColumn.removeFromTop(controlHeight);
    auto chordComboArea = soundTestRow.removeFromLeft(88);
    soundTestChordCombo.setBounds(chordComboArea);
    soundTestRow.removeFromLeft(controlGap);
    soundTestButton.setBounds(soundTestRow);

    auto bottomInfoArea = leftColumn.removeFromBottom(56 + sectionGap + controlHeight);
    auto selectedPatchHeaderRow = bottomInfoArea.removeFromTop(14);
    selectedPatchHeaderLabel.setBounds(selectedPatchHeaderRow);
    selectedPatchValuesLabel.setBounds(selectedPatchHeaderRow);
    selectedPatchNameLabel.setBounds(bottomInfoArea.removeFromTop(34));
    refreshSelectedPatchDetails();
    bottomInfoArea.removeFromTop(sectionGap);

    auto navigationRow = bottomInfoArea.removeFromTop(controlHeight);
    const auto navigationWidth = (navigationRow.getWidth() - controlGap) / 2;
    previousPatchButton.setBounds(navigationRow.removeFromLeft(navigationWidth));
    navigationRow.removeFromLeft(controlGap);
    nextPatchButton.setBounds(navigationRow);

    patchList.setBounds(rightColumn);
}

void PatchSelectorAudioProcessorEditor::refreshView()
{
    refreshLibraryList();

    deviceLabel.setText({}, juce::dontSendNotification);

    categoryCombo.clear(juce::dontSendNotification);
    const auto categories = audioProcessor.getCategories();
    for (int i = 0; i < categories.size(); ++i)
        categoryCombo.addItem(categories[i], i + 1);

    if (categoryCombo.getNumItems() > 0)
        categoryCombo.setSelectedItemIndex(0, juce::dontSendNotification);

    rebuildVisiblePatchList();

    if (audioProcessor.getLastErrorMessage().isNotEmpty())
        updateStatus(audioProcessor.getLastErrorMessage(), true);
    else if (audioProcessor.getAvailableLibraryNames().isEmpty())
        updateStatus("No libraries found. Add JSON files to the library folder.", false);
    else
        updateStatus("Ready", false);
}

void PatchSelectorAudioProcessorEditor::rebuildVisiblePatchList()
{
    audioProcessor.setActiveFilterState(categoryCombo.getText(), searchEditor.getText());
    visiblePatchIndices = audioProcessor.buildVisiblePatchIndices(categoryCombo.getText(), searchEditor.getText());
    patchList.updateContent();
    syncSelectionFromProcessor();
}

void PatchSelectorAudioProcessorEditor::syncSelectionFromProcessor()
{
    const auto selectedRealIndex = audioProcessor.getSelectedPatchIndex();
    auto it = std::find(visiblePatchIndices.begin(), visiblePatchIndices.end(), selectedRealIndex);

    if (it == visiblePatchIndices.end())
    {
        patchList.deselectAllRows();
        refreshSelectedPatchDetails();
        return;
    }

    const auto visibleIndex = (int) std::distance(visiblePatchIndices.begin(), it);
    patchList.selectRow(visibleIndex);
    refreshSelectedPatchDetails();
}

void PatchSelectorAudioProcessorEditor::updateStatus(const juce::String& message, bool isError)
{
    statusLabel.setText(isError ? message : juce::String(), juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, isError ? juce::Colours::orange : juce::Colours::lightgreen);
    resized();
}

void PatchSelectorAudioProcessorEditor::refreshLibraryList()
{
    audioProcessor.refreshAvailableLibraries();

    libraryCombo.clear(juce::dontSendNotification);
    const auto names = audioProcessor.getAvailableLibraryNames();
    for (int i = 0; i < names.size(); ++i)
        libraryCombo.addItem(names[i], i + 1);

    const auto loadedIndex = audioProcessor.getLoadedLibraryIndex();
    if (loadedIndex >= 0)
        libraryCombo.setSelectedItemIndex(loadedIndex, juce::dontSendNotification);
    else if (libraryCombo.getNumItems() > 0)
        libraryCombo.setSelectedItemIndex(0, juce::dontSendNotification);
}

void PatchSelectorAudioProcessorEditor::refreshSelectedPatchDetails()
{
    const auto* library = audioProcessor.getPatchLibrary();
    const auto selectedRealIndex = audioProcessor.getSelectedPatchIndex();

    if (library == nullptr
        || selectedRealIndex < 0
        || selectedRealIndex >= (int) library->patches.size())
    {
        selectedPatchHeaderLabel.setText("Category", juce::dontSendNotification);
        selectedPatchNameLabel.setText("No patch selected", juce::dontSendNotification);
        selectedPatchValuesLabel.setText({}, juce::dontSendNotification);
        return;
    }

    const auto& patch = library->patches[(size_t) selectedRealIndex];
    selectedPatchHeaderLabel.setText(patch.category, juce::dontSendNotification);
    selectedPatchNameLabel.setText(getSelectedPatchDisplayName(patch.name), juce::dontSendNotification);
    selectedPatchValuesLabel.setText("MSB " + juce::String(patch.msb)
                                     + "  LSB " + juce::String(patch.lsb)
                                     + "  PRG " + juce::String(patch.program),
                                     juce::dontSendNotification);
}

juce::String PatchSelectorAudioProcessorEditor::getSelectedPatchDisplayName(const juce::String& patchName) const
{
    const auto availableWidth = juce::jmax(0, selectedPatchNameLabel.getWidth() - 4);

    if (availableWidth <= 0)
        return patchName;

    auto font = selectedPatchNameLabel.getFont();
    const auto stringWidth = [&font] (const juce::String& text)
    {
        juce::GlyphArrangement glyphs;
        glyphs.addLineOfText(font, text, 0.0f, 0.0f);
        return glyphs.getBoundingBox(0, text.length(), true).getWidth();
    };

    if (stringWidth(patchName) <= (float) availableWidth)
        return patchName;

    constexpr auto ellipsis = "...";
    const auto ellipsisWidth = stringWidth(ellipsis);

    if (ellipsisWidth >= (float) availableWidth)
        return ellipsis;

    juce::String truncated;

    for (int i = 0; i < patchName.length(); ++i)
    {
        const auto candidate = truncated + patchName.substring(i, i + 1);

        if (stringWidth(candidate) + ellipsisWidth > (float) availableWidth)
            break;

        truncated = candidate;
    }

    return truncated + ellipsis;
}

void PatchSelectorAudioProcessorEditor::stepSelectedPatch(int direction)
{
    if (visiblePatchIndices.empty())
        return;

    const auto selectedRealIndex = audioProcessor.getSelectedPatchIndex();
    auto visibleIterator = std::find(visiblePatchIndices.begin(), visiblePatchIndices.end(), selectedRealIndex);

    int visibleIndex = 0;

    if (visibleIterator != visiblePatchIndices.end())
        visibleIndex = (int) std::distance(visiblePatchIndices.begin(), visibleIterator);

    visibleIndex = juce::jlimit(0,
                                (int) visiblePatchIndices.size() - 1,
                                visibleIndex + direction);

    patchList.selectRow(visibleIndex);
    patchList.scrollToEnsureRowIsOnscreen(visibleIndex);
}

int PatchSelectorAudioProcessorEditor::getNumRows()
{
    return (int) visiblePatchIndices.size();
}

void PatchSelectorAudioProcessorEditor::paintListBoxItem(int rowNumber,
                                                         juce::Graphics& g,
                                                         int width,
                                                         int height,
                                                         bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= (int) visiblePatchIndices.size())
        return;

    const auto* library = audioProcessor.getPatchLibrary();
    if (library == nullptr)
        return;

    const auto realIndex = visiblePatchIndices[(size_t) rowNumber];
    const auto& patch = library->patches[(size_t) realIndex];

    if (rowIsSelected)
        g.fillAll(juce::Colours::dodgerblue.withAlpha(0.35f));

    constexpr int horizontalPadding = 8;
    constexpr int columnGap = 10;
    const auto contentWidth = juce::jmax(0, width - (horizontalPadding * 2));
    const auto equalColumnWidth = juce::jmax(80, (contentWidth - (columnGap * 2)) / 3);

    auto rowArea = juce::Rectangle<int>(horizontalPadding, 0, contentWidth, height);
    auto nameArea = rowArea.removeFromLeft(equalColumnWidth);
    rowArea.removeFromLeft(columnGap);
    auto categoryArea = rowArea.removeFromLeft(equalColumnWidth);
    rowArea.removeFromLeft(columnGap);
    auto codeArea = rowArea;

    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText(patch.name, nameArea, juce::Justification::centredLeft, true);

    g.setColour(juce::Colours::lightgrey);
    g.drawText(patch.category, categoryArea, juce::Justification::centredLeft, true);

    const auto code = "MSB " + juce::String(patch.msb)
                    + "  LSB " + juce::String(patch.lsb)
                    + "  PRG " + juce::String(patch.program);
    g.drawText(code, codeArea, juce::Justification::centredRight, true);
}

void PatchSelectorAudioProcessorEditor::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected < 0 || lastRowSelected >= (int) visiblePatchIndices.size())
        return;

    const auto realIndex = visiblePatchIndices[(size_t) lastRowSelected];
    if (audioProcessor.selectPatchByRealIndex(realIndex, audioProcessor.getAutoSendOnSelection()))
        updateStatus(audioProcessor.getAutoSendOnSelection() ? "Selected patch queued" : "Selected patch", false);
}

void PatchSelectorAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &openFolderButton)
    {
        audioProcessor.revealLibraryFolder();
        updateStatus("Opened library folder", false);
        return;
    }

    if (button == &rescanButton)
    {
        refreshLibraryList();
        refreshView();
        updateStatus("Library folder rescanned", false);
        return;
    }

    if (button == &sendButton)
    {
        audioProcessor.sendSelectedPatchNow();
        updateStatus("Patch send queued", false);
        return;
    }

    if (button == &previousPatchButton)
    {
        stepSelectedPatch(-1);
        return;
    }

    if (button == &nextPatchButton)
    {
        stepSelectedPatch(1);
    }
}

void PatchSelectorAudioProcessorEditor::buttonStateChanged(juce::Button* button)
{
    if (button != &soundTestButton)
        return;

    if (button->isDown())
        audioProcessor.startSoundTest();
    else
        audioProcessor.stopSoundTest();
}

void PatchSelectorAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &libraryCombo)
    {
        juce::String error;
        if (audioProcessor.loadAvailableLibraryByIndex(libraryCombo.getSelectedItemIndex(), error))
        {
            refreshView();
            updateStatus("Library loaded: " + libraryCombo.getText(), false);
        }
        else if (error.isNotEmpty())
        {
            updateStatus(error, true);
        }

        return;
    }

    if (comboBoxThatHasChanged == &categoryCombo)
        rebuildVisiblePatchList();

    if (comboBoxThatHasChanged == &soundTestChordCombo)
        audioProcessor.setSoundTestChordIndex(soundTestChordCombo.getSelectedItemIndex());
}

void PatchSelectorAudioProcessorEditor::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == &searchEditor)
        rebuildVisiblePatchList();
}

void PatchSelectorAudioProcessorEditor::timerCallback()
{
    const auto selectedRealIndex = audioProcessor.getSelectedPatchIndex();
    const auto selectedVisibleRow = patchList.getSelectedRow();

    if (selectedVisibleRow >= 0
        && selectedVisibleRow < (int) visiblePatchIndices.size()
        && visiblePatchIndices[(size_t) selectedVisibleRow] == selectedRealIndex)
        return;

    syncSelectionFromProcessor();

    auto it = std::find(visiblePatchIndices.begin(), visiblePatchIndices.end(), selectedRealIndex);
    if (it != visiblePatchIndices.end())
        patchList.scrollToEnsureRowIsOnscreen((int) std::distance(visiblePatchIndices.begin(), it));
}
