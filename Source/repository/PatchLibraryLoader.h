#pragma once

#include "../model/PatchLibrary.h"
#include <optional>

class PatchLibraryLoader
{
public:
    static std::optional<PatchLibrary> loadFromFile(const juce::File& file, juce::String& error);
    static std::optional<PatchLibrary> loadFromJson(const juce::var& json, juce::String& error);

private:
    static bool parseDevice(const juce::var& deviceVar, DeviceDefinition& out, juce::String& error);
    static bool parsePatches(const juce::var& patchesVar, std::vector<PatchDefinition>& out, int programBase, juce::String& error);

    static bool readRequiredInt(const juce::DynamicObject& object, const juce::Identifier& property, int minValue, int maxValue, int& output, juce::String& error, const juce::String& context);
    static bool readRequiredString(const juce::DynamicObject& object, const juce::Identifier& property, juce::String& output, juce::String& error, const juce::String& context);
};
