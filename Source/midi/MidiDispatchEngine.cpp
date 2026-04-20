#include "MidiDispatchEngine.h"

juce::Array<juce::MidiMessage> MidiDispatchEngine::createPatchSelectMessages(const PatchDefinition& patch,
                                                                              int midiChannel,
                                                                              int programBase)
{
    juce::Array<juce::MidiMessage> result;

    const auto ch = juce::jlimit(1, 16, midiChannel);
    const auto midiProgram = juce::jlimit(0, 127, toMidiProgramNumber(patch.program, programBase));

    result.add(juce::MidiMessage::controllerEvent(ch, 0, juce::jlimit(0, 127, patch.msb)));
    result.add(juce::MidiMessage::controllerEvent(ch, 32, juce::jlimit(0, 127, patch.lsb)));
    result.add(juce::MidiMessage::programChange(ch, midiProgram));

    return result;
}

int MidiDispatchEngine::toMidiProgramNumber(int programFromJson, int programBase)
{
    return programFromJson - programBase;
}
