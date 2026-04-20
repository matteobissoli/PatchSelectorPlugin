#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "../model/PatchLibrary.h"

class MidiDispatchEngine
{
public:
    static juce::Array<juce::MidiMessage> createPatchSelectMessages(const PatchDefinition& patch,
                                                                    int midiChannel,
                                                                    int programBase);

    static int toMidiProgramNumber(int programFromJson, int programBase);
};
