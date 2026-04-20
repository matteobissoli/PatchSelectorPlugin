# Patch Selector

English | [Italiano](./README.it.md)

Patch Selector is a JUCE-based MIDI effect plugin for macOS hosts such as Logic Pro and MainStage.
It loads JSON patch libraries, lets the user browse patches by category and name, and emits MIDI Bank Select (`CC0` / `CC32`) plus Program Change messages for external hardware synthesizers.

This repository is structured to be practical to build locally and ready to evolve as an open source project.

## Features

- AU, VST3, and Standalone targets built with CMake and JUCE
- JSON patch-library loading
- Dedicated library-folder workflow with rescan support
- Patch filtering by category and text search
- Previous / next patch navigation
- Persistent plugin state
- Automatic and manual MIDI patch send workflows
- Sound test with selectable chord and MIDI channel

## Status

The project is usable, but still evolving.

Current priorities are:

- reliability inside Logic Pro and MainStage
- patch-library compatibility improvements
- workflow quality for external hardware synths
- UI refinement for larger libraries

## Platform Support

- macOS
- AU, VST3, Standalone
- Xcode toolchain

The main target workflow is a MIDI effect on a host track that controls an external synth.

## Repository Layout

```text
Source/
  PluginProcessor.*         Plugin logic and state
  PluginEditor.*            JUCE editor UI
  midi/                     MIDI dispatch logic
  model/                    Patch-library model types
  repository/               JSON loading and validation
resources/                  Runtime resources and library assets
JUCE/                       JUCE dependency vendored in-repo
build-xcode/                Local Xcode/CMake build directory
```

## Build Requirements

- macOS
- CMake 3.22+
- Xcode

JUCE is vendored in this repository, so no separate `find_package(JUCE ...)` installation is required.

## Build

Configure:

```bash
cmake -S . -B build-xcode -G Xcode
```

Build:

```bash
cmake --build build-xcode --config Release
```

Artifacts are generated for:

- Audio Unit (`.component`)
- VST3 (`.vst3`)
- Standalone app

## Usage

1. Build the project.
2. Open the standalone app or load the AU/VST3 plugin in a host.
3. Use the folder button to open the library directory.
4. Add one or more patch-library JSON files.
5. Rescan libraries.
6. Select a library, browse patches, and send the desired bank/program messages.

## Library Folder

The plugin scans JSON libraries from:

- `~/Library/Application Support/PatchSelectorPlugin/Libraries`

## JSON Library Format

Minimal example:

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

The loader is intentionally tolerant and supports several aliases and relaxed formats for compatibility with real-world libraries.

## Generate Libraries With AI

You can use ChatGPT or another generative AI tool to produce a patch-library JSON file for Patch Selector.

Recommended workflow:

1. Collect the patch list for your synth from a manual, vendor PDF, spreadsheet, or trusted web source.
2. Ask the AI to normalize that data into the JSON format expected by this plugin.
3. Review the generated `msb`, `lsb`, `program`, `name`, and `category` values carefully before using the file in a live setup.
4. Save the result as a `.json` file inside:
   `~/Library/Application Support/PatchSelectorPlugin/Libraries`
5. Open the plugin and press `Rescan`.

Suggested prompt in English:

```text
Create a valid JSON patch library for the Patch Selector plugin.

Output only raw JSON, with no Markdown fences and no explanation.

Requirements:
- The root object must contain a "device" object and a "patches" array.
- The "device" object must include:
  - "manufacturer"
  - "model"
  - "defaultMidiChannel"
  - "programBase"
- Each item in "patches" must include:
  - "msb" as integer
  - "lsb" as integer
  - "program" as integer
  - "name" as string
  - "category" as string
- Use category names that are useful for filtering, such as Piano, EP, Organ, Pad, Strings, Brass, Lead, Bass, Drum, Synth, FX, Voice, Guitar, or Misc.
- Preserve the original patch names exactly when possible.
- Do not invent patches that are not present in the source material.
- If bank numbering or program numbering is ambiguous, keep the most likely values and add a top-level string field named "notes" explaining the assumption.
- Assume programBase is 1 unless the source clearly uses zero-based program numbering.
- Return strictly valid JSON.

Use this synth data as source material:
[PASTE HERE THE PATCH LIST, BANK TABLE, OR MANUAL EXCERPT]
```

If you already know the exact synth model, you can make the prompt stricter by naming it directly, for example:

```text
Create a Patch Selector JSON library for a Yamaha Motif ES from the following source material...
```

Practical advice:

- Ask the model to work from a pasted table, manual excerpt, or OCR text rather than from memory.
- For large synths, generate one bank at a time and merge only after checking the output.
- If the source material is messy, ask the model first to extract a clean table, then ask for the final JSON in a second step.
- Always spot-check a few patches on the real instrument before trusting the whole library.

## Design Notes

- The plugin does not open hardware MIDI ports directly.
  The host is expected to route MIDI to the destination instrument.
- The plugin does not capture hardware audio.
  In Logic Pro, pair it with External Instrument or a dedicated return path.
- Generated MIDI messages still require an explicit MIDI channel.
  They do not automatically inherit the host track channel unless the plugin derives that channel from incoming MIDI.

## Open Source

- License: [MIT](./LICENSE)
- Contributing guide: [CONTRIBUTING.md](./CONTRIBUTING.md)
- Code of conduct: [CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md)
- Security policy: [SECURITY.md](./SECURITY.md)

## Contributing

Bug reports, parser compatibility improvements, workflow feedback, and focused pull requests are welcome.

Before opening a larger contribution, read [CONTRIBUTING.md](./CONTRIBUTING.md).

## Roadmap Ideas

- SysEx support
- richer patch metadata
- import tools for more vendor formats
- better host-specific validation
- regression tests around parsing and MIDI dispatch behavior

## License

This project is released under the [MIT License](./LICENSE).
