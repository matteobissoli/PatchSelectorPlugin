# PatchSelectorPlugin

PatchSelectorPlugin is a JUCE-based MIDI effect plugin for macOS hosts such as Logic Pro and MainStage.
It loads JSON patch libraries, lets the user browse patches by category and name, and emits MIDI Bank Select (CC0/CC32) plus Program Change messages for an external hardware synthesizer.

## What is implemented

- JUCE plugin target configured as MIDI effect
- AU, VST3 and Standalone targets in CMake
- JSON patch library parser with validation
- Dedicated library folder discovery and rescanning
- Patch browser UI with:
  - library selection from a dedicated folder
  - category filter
  - text search
  - MIDI channel selector
  - patch list
  - send button
- Persistent plugin state for:
  - selected library path
  - selected patch index
  - MIDI channel
  - auto-send on selection
  - resend on preset/project load
  - resend on transport start
- MIDI message queue flushed from `processBlock`
- Transport edge detection to resend the selected patch when playback starts

## Dedicated library folder

The plugin scans JSON libraries from:

- macOS: `~/Library/Application Support/PatchSelectorPlugin/Libraries`

Use the **Library Folder** button to open the folder in Finder, add one or more JSON library files, then press **Rescan**.

## JSON format

```json
{
  "device": {
    "manufacturer": "Generic",
    "model": "My Synth",
    "defaultMidiChannel": 1,
    "programBase": 1
  },
  "patches": [
    {
      "msb": 0,
      "lsb": 0,
      "program": 1,
      "name": "Grand Piano",
      "category": "Piano"
    }
  ]
}
```

## Build requirements

- CMake 3.22+
- A local JUCE installation exportable through `find_package(JUCE CONFIG REQUIRED)`
- Xcode on macOS for AU builds

Example configure/build:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/JUCE
cmake --build build --config Release
```

## Notes

- The plugin intentionally does not open hardware MIDI ports itself.
  The host should route MIDI from the plugin to the external instrument.
- The plugin intentionally does not capture hardware audio.
  In Logic Pro, pair it with External Instrument or a dedicated audio return path.
- The current version sends only CC0, CC32 and Program Change.
  SysEx and advanced multi/performance modes are not implemented yet.
