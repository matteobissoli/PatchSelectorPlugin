#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class PatchSelectorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Button::Listener,
                                         private juce::ComboBox::Listener,
                                         private juce::TextEditor::Listener,
                                         private juce::ListBoxModel
{
public:
    explicit PatchSelectorAudioProcessorEditor(PatchSelectorAudioProcessor&);
    ~PatchSelectorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    PatchSelectorAudioProcessor& audioProcessor;

    juce::Label deviceLabel;
    juce::Label statusLabel;
    juce::Label libraryLabel;
    juce::Label categoryLabel;
    juce::Label searchLabel;
    juce::Label midiChannelLabel;
    juce::Label selectedPatchHeaderLabel;
    juce::Label selectedPatchValuesLabel;
    juce::Label selectedPatchNameLabel;

    juce::TextButton openFolderButton { "Library Folder" };
    juce::TextButton rescanButton { "Rescan" };
    juce::TextButton sendButton { "Test" };
    juce::TextButton soundTestButton { "Sound Test" };
    juce::TextButton previousPatchButton { "Previous" };
    juce::TextButton nextPatchButton { "Next" };
    juce::ComboBox libraryCombo;
    juce::ComboBox categoryCombo;
    juce::ComboBox midiChannelCombo;
    juce::ComboBox soundTestChordCombo;
    juce::TextEditor searchEditor;
    juce::ToggleButton autoSendToggle { "Auto send on selection" };
    juce::ToggleButton sendOnLoadToggle { "Resend on preset/project load" };
    juce::ToggleButton resendOnTransportToggle { "Resend on transport start" };
    juce::ListBox patchList { "Patch List", this };

    std::vector<int> visiblePatchIndices;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoSendAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sendOnLoadAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> resendOnTransportAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> midiChannelAttachment;

    void refreshView();
    void rebuildVisiblePatchList();
    void syncSelectionFromProcessor();
    void updateStatus(const juce::String& message, bool isError);
    void refreshLibraryList();
    void stepSelectedPatch(int direction);
    void refreshSelectedPatchDetails();
    juce::String getSelectedPatchDisplayName(const juce::String& patchName) const;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics&, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void buttonClicked(juce::Button* button) override;
    void buttonStateChanged(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchSelectorAudioProcessorEditor)
};
