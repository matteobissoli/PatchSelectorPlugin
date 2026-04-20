# Contributing

Thanks for contributing to Patch Selector.

## Before You Start

- Keep changes focused and easy to review.
- Prefer fixes that solve real host workflows.
- Avoid unrelated refactors in the same pull request.
- Open an issue first if the change is large, architectural, or changes user workflow significantly.

## Good Contributions

- bug fixes
- parser compatibility improvements
- host-specific MIDI workflow fixes
- UI improvements with clear usability gains
- test coverage
- documentation improvements

## Pull Request Guidelines

1. Make one logical change per pull request.
2. Explain the user-visible problem being solved.
3. Describe how you validated the change.
4. Include screenshots or notes for UI changes when useful.
5. Do not commit generated build products.

## Code Style

- Follow the existing C++ and JUCE style already used in the repository.
- Prefer direct, readable code over unnecessary abstraction.
- Keep UI changes coherent with the current plugin layout.
- Add comments only where behavior would otherwise be non-obvious.

## Validation

When possible, validate at least one of the following:

- project builds successfully in Xcode
- plugin opens in Standalone
- plugin opens in Logic Pro or MainStage
- affected JSON libraries still parse correctly
- affected MIDI behavior still works as expected

## Issues

When reporting a bug, include:

- host application and version
- plugin format used (`AU`, `VST3`, `Standalone`)
- macOS version
- exact reproduction steps
- library file or a reduced example if parsing is involved

## Scope

Please avoid:

- unrelated whitespace-only changes
- mass formatting in untouched files
- generated build output
- vendor changes unless they are required for the fix

## License

By contributing to this repository, you agree that your contributions will be licensed under the MIT License.
