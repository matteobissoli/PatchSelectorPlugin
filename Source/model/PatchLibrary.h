#pragma once

#include <juce_core/juce_core.h>
#include <vector>

struct PatchDefinition
{
    int msb = 0;
    int lsb = 0;
    int program = 1;
    juce::String name;
    juce::String category;
    juce::String subcategory;
    juce::StringArray tags;
    juce::String comment;
};

struct DeviceDefinition
{
    juce::String manufacturer;
    juce::String model;
    int defaultMidiChannel = 1;
    int programBase = 1;
};

struct PatchLibrary
{
    DeviceDefinition device;
    std::vector<PatchDefinition> patches;

    juce::String getDisplayName() const
    {
        auto combined = (device.manufacturer + " " + device.model).trim();
        return combined.isNotEmpty() ? combined : "Unknown Device";
    }
};
