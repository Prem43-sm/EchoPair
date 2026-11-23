# EchoPair

**One Sound. Multiple Ears.**

EchoPair is a Windows application project for routing audio to multiple
existing playback devices without replacing Windows Bluetooth drivers.

The current build enumerates actual Windows playback endpoints using Core
Audio and can play a 440 Hz test tone on one active endpoint when requested.
System-audio capture, multi-device routing, synchronization, and latency
measurements are not implemented. No devices or measurements are simulated.

## Build

See [BUILD.md](BUILD.md) and [SETUP.md](SETUP.md) for requirements and
commands.

## Project status

See [LIMITATIONS.md](LIMITATIONS.md). Endpoint discovery and subsequent audio
features are being implemented in separate, testable phases.
