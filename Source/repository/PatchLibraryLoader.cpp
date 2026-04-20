#include "PatchLibraryLoader.h"

namespace
{
constexpr int defaultProgramBase = 1;
constexpr auto defaultCategory = "Uncategorized";

const juce::DynamicObject* asObject(const juce::var& value)
{
    return value.getDynamicObject();
}

juce::String contextForPatch(int index)
{
    return "Patch " + juce::String(index + 1);
}

juce::var getFirstPresentProperty(const juce::DynamicObject& object, std::initializer_list<const char*> propertyNames)
{
    for (const auto* propertyName : propertyNames)
    {
        const juce::Identifier property(propertyName);

        if (object.hasProperty(property))
            return object.getProperty(property);
    }

    return {};
}

juce::String getFirstPresentString(const juce::DynamicObject& object, std::initializer_list<const char*> propertyNames)
{
    return getFirstPresentProperty(object, propertyNames).toString().trim();
}

bool tryParseIntValue(const juce::var& value, int& output)
{
    if (value.isInt() || value.isInt64() || value.isDouble() || value.isBool())
    {
        output = static_cast<int>(value);
        return true;
    }

    const auto text = value.toString().trim();

    if (text.isEmpty())
        return false;

    bool isValid = false;
    const auto parsed = text.getIntValue();

    if (parsed != 0 || text == "0" || text == "+0" || text == "-0")
        isValid = true;
    else if (text == "1" || text == "+1" || text == "-1")
        isValid = true;

    if (! isValid)
        return false;

    output = parsed;
    return true;
}

bool readIntWithAliases(const juce::DynamicObject& object,
                        std::initializer_list<const char*> propertyNames,
                        int minValue,
                        int maxValue,
                        int& output,
                        juce::String& error,
                        const juce::String& context,
                        bool required)
{
    for (const auto* propertyName : propertyNames)
    {
        const juce::Identifier property(propertyName);

        if (! object.hasProperty(property))
            continue;

        const auto value = object.getProperty(property);
        int intValue = 0;

        if (! tryParseIntValue(value, intValue))
        {
            error = context + ": field '" + property.toString() + "' must be numeric.";
            return false;
        }

        if (intValue < minValue || intValue > maxValue)
        {
            error = context + ": field '" + property.toString() + "' must be between "
                  + juce::String(minValue) + " and " + juce::String(maxValue) + ".";
            return false;
        }

        output = intValue;
        return true;
    }

    if (required)
    {
        error = context + ": missing required numeric field.";
        return false;
    }

    return true;
}

bool readStringWithAliases(const juce::DynamicObject& object,
                           std::initializer_list<const char*> propertyNames,
                           juce::String& output,
                           juce::String& error,
                           const juce::String& context,
                           bool required)
{
    for (const auto* propertyName : propertyNames)
    {
        const juce::Identifier property(propertyName);

        if (! object.hasProperty(property))
            continue;

        output = object.getProperty(property).toString().trim();

        if (output.isEmpty())
        {
            error = context + ": field '" + property.toString() + "' cannot be empty.";
            return false;
        }

        return true;
    }

    if (required)
    {
        error = context + ": missing required text field.";
        return false;
    }

    output.clear();
    return true;
}
}

std::optional<PatchLibrary> PatchLibraryLoader::loadFromFile(const juce::File& file, juce::String& error)
{
    if (! file.existsAsFile())
    {
        error = "Library file not found: " + file.getFullPathName();
        return std::nullopt;
    }

    auto rawJson = file.loadFileAsString();

    if (rawJson.startsWithChar(juce::juce_wchar(0xfeff)))
        rawJson = rawJson.substring(1);

    auto parsed = juce::JSON::parse(rawJson);

    if (parsed.isVoid())
    {
        error = "Invalid JSON in file: " + file.getFileName();
        return std::nullopt;
    }

    return loadFromJson(parsed, error);
}

std::optional<PatchLibrary> PatchLibraryLoader::loadFromJson(const juce::var& json, juce::String& error)
{
    const auto* rootObject = asObject(json);
    const auto* rootArray = json.getArray();

    if (rootObject == nullptr && rootArray == nullptr)
    {
        error = "JSON root must be an object or an array of patches.";
        return std::nullopt;
    }

    PatchLibrary library;

    library.device.programBase = defaultProgramBase;

    if (rootObject != nullptr)
    {
        juce::var deviceVar = rootObject->getProperty("device");

        if (deviceVar.isVoid())
            deviceVar = rootObject->getProperty("instrument");

        if (deviceVar.isVoid())
            deviceVar = rootObject->getProperty("synth");

        if (! deviceVar.isVoid())
        {
            if (! parseDevice(deviceVar, library.device, error))
                return std::nullopt;
        }
        else
        {
            DeviceDefinition rootDevice;

            if (! parseDevice(json, rootDevice, error))
            {
                error.clear();
            }
            else
            {
                library.device = std::move(rootDevice);
            }
        }

        auto patchesVar = rootObject->getProperty("patches");

        if (patchesVar.isVoid())
            patchesVar = rootObject->getProperty("presets");

        if (patchesVar.isVoid())
            patchesVar = rootObject->getProperty("programs");

        if (patchesVar.isVoid())
            patchesVar = rootObject->getProperty("sounds");

        if (! patchesVar.isVoid())
        {
            if (! parsePatches(patchesVar, library.patches, library.device.programBase, error))
                return std::nullopt;
        }
        else
        {
            error = "Missing patch array. Expected one of: 'patches', 'presets', 'programs', 'sounds'.";
            return std::nullopt;
        }
    }
    else
    {
        if (! parsePatches(json, library.patches, library.device.programBase, error))
            return std::nullopt;
    }

    return library;
}

bool PatchLibraryLoader::parseDevice(const juce::var& deviceVar, DeviceDefinition& out, juce::String& error)
{
    const auto* deviceObject = asObject(deviceVar);
    if (deviceObject == nullptr)
    {
        error = "Missing or invalid 'device' object.";
        return false;
    }

    out.manufacturer = getFirstPresentString(*deviceObject, { "manufacturer", "brand", "vendor", "make" });
    out.model = getFirstPresentString(*deviceObject, { "model", "device", "instrument", "name" });

    if (getFirstPresentProperty(*deviceObject, { "defaultMidiChannel", "midiChannel", "channel", "defaultChannel" }).isVoid())
    {
        out.defaultMidiChannel = 1;
    }
    else
    {
        if (! readIntWithAliases(*deviceObject,
                                 { "defaultMidiChannel", "midiChannel", "channel", "defaultChannel" },
                                 1,
                                 16,
                                 out.defaultMidiChannel,
                                 error,
                                 "Device",
                                 false))
            return false;
    }

    out.programBase = defaultProgramBase;
    if (! getFirstPresentProperty(*deviceObject, { "programBase", "program_base", "programIndexBase", "zeroBasedPrograms" }).isVoid())
    {
        if (deviceObject->hasProperty("zeroBasedPrograms"))
        {
            const auto zeroBasedVar = deviceObject->getProperty("zeroBasedPrograms");

            if (zeroBasedVar.isBool())
                out.programBase = static_cast<bool>(zeroBasedVar) ? 0 : 1;
            else
            {
                int zeroBasedInt = 0;

                if (! tryParseIntValue(zeroBasedVar, zeroBasedInt))
                {
                    error = "Device: field 'zeroBasedPrograms' must be boolean or numeric.";
                    return false;
                }

                out.programBase = zeroBasedInt != 0 ? 0 : 1;
            }
        }
        else if (! readIntWithAliases(*deviceObject,
                                      { "programBase", "program_base", "programIndexBase" },
                                      0,
                                      1,
                                      out.programBase,
                                      error,
                                      "Device",
                                      false))
        {
            return false;
        }
    }

    return true;
}

bool PatchLibraryLoader::parsePatches(const juce::var& patchesVar, std::vector<PatchDefinition>& out, int programBase, juce::String& error)
{
    if (! patchesVar.isArray())
    {
        error = "Missing or invalid 'patches' array.";
        return false;
    }

    const auto* patchesArray = patchesVar.getArray();
    if (patchesArray == nullptr || patchesArray->isEmpty())
    {
        error = "The 'patches' array is empty.";
        return false;
    }

    const int programMin = (programBase == 1 ? 1 : 0);
    const int programMax = (programBase == 1 ? 128 : 127);

    out.clear();
    out.reserve((size_t) patchesArray->size());

    for (int i = 0; i < patchesArray->size(); ++i)
    {
        const auto& patchVar = patchesArray->getReference(i);
        const auto* patchObject = asObject(patchVar);
        if (patchObject == nullptr)
        {
            error = contextForPatch(i) + ": must be an object.";
            return false;
        }

        PatchDefinition patch;
        const auto context = contextForPatch(i);

        if (! readIntWithAliases(*patchObject,
                                 { "msb", "bankMSB", "bankMsb", "bank_msb" },
                                 0,
                                 127,
                                 patch.msb,
                                 error,
                                 context,
                                 true))
            return false;

        if (! readIntWithAliases(*patchObject,
                                 { "lsb", "bankLSB", "bankLsb", "bank_lsb" },
                                 0,
                                 127,
                                 patch.lsb,
                                 error,
                                 context,
                                 true))
            return false;

        if (! readIntWithAliases(*patchObject,
                                 { "program", "programNumber", "number", "patch", "pc", "program_change" },
                                 programMin,
                                 programMax,
                                 patch.program,
                                 error,
                                 context,
                                 true))
            return false;

        if (! readStringWithAliases(*patchObject,
                                    { "name", "patchName", "title", "presetName" },
                                    patch.name,
                                    error,
                                    context,
                                    true))
            return false;

        if (! readStringWithAliases(*patchObject,
                                    { "category", "group", "type", "bank" },
                                    patch.category,
                                    error,
                                    context,
                                    false))
        {
            return false;
        }

        if (patch.category.isEmpty())
            patch.category = defaultCategory;

        patch.subcategory = getFirstPresentString(*patchObject, { "subcategory", "subCategory", "sub_category", "subgroup" });

        patch.comment = getFirstPresentString(*patchObject, { "comment", "description", "notes" });

        const auto tagsVar = getFirstPresentProperty(*patchObject, { "tags", "keywords" });
        if (! tagsVar.isVoid())
        {
            if (tagsVar.isArray())
            {
                for (const auto& tag : *tagsVar.getArray())
                {
                    const auto tagText = tag.toString().trim();

                    if (tagText.isNotEmpty())
                        patch.tags.add(tagText);
                }
            }
            else
            {
                const auto tagText = tagsVar.toString().trim();

                if (tagText.isNotEmpty())
                    patch.tags.addTokens(tagText, ",;", "\"");
            }
        }

        out.push_back(std::move(patch));
    }

    return true;
}

bool PatchLibraryLoader::readRequiredInt(const juce::DynamicObject& object,
                                         const juce::Identifier& property,
                                         int minValue,
                                         int maxValue,
                                         int& output,
                                         juce::String& error,
                                         const juce::String& context)
{
    if (! object.hasProperty(property))
    {
        error = context + ": missing field '" + property.toString() + "'.";
        return false;
    }

    const auto value = object.getProperty(property);
    if (! value.isInt() && ! value.isInt64() && ! value.isDouble())
    {
        error = context + ": field '" + property.toString() + "' must be numeric.";
        return false;
    }

    const auto intValue = static_cast<int>(value);
    if (intValue < minValue || intValue > maxValue)
    {
        error = context + ": field '" + property.toString() + "' must be between "
              + juce::String(minValue) + " and " + juce::String(maxValue) + ".";
        return false;
    }

    output = intValue;
    return true;
}

bool PatchLibraryLoader::readRequiredString(const juce::DynamicObject& object,
                                            const juce::Identifier& property,
                                            juce::String& output,
                                            juce::String& error,
                                            const juce::String& context)
{
    if (! object.hasProperty(property))
    {
        error = context + ": missing field '" + property.toString() + "'.";
        return false;
    }

    output = object.getProperty(property).toString().trim();
    if (output.isEmpty())
    {
        error = context + ": field '" + property.toString() + "' cannot be empty.";
        return false;
    }

    return true;
}
