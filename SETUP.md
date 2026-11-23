# Development setup

## Checked environment

The initial environment inspection was performed on 2026-11-23:

- Windows 11 Home Single Language, x64, build 26300
- Visual Studio Build Tools 2022, version 17.14.42
- MSVC x64 compiler, version 19.44.35229
- Windows SDK versions 10.0.22621.0 and 10.0.26100.0
- CMake 4.2.3
- Git 2.51.1
- No WDK installation was detected during the initial inspection

After explicit authorization, WDK 10.1.26100.6584 was installed and verified
through WinGet. Its headers and build targets are present. The Visual Studio
Build Tools driver platform-toolset integration has not yet been verified.
Secure Boot, test-signing, and driver-installation settings were not changed.

Windows reports audio endpoint devices on this machine. Their presence does
not validate playback, Bluetooth classification, or multi-device routing.
Those capabilities must be tested separately against the Windows Core Audio
APIs and physical hardware.

## Requirements

- Windows 10 22H2 or newer, x64
- Visual Studio 2022 C++ build tools and a Windows 10/11 SDK
- CMake 3.20 or newer

The current user-mode endpoint discovery and WASAPI work does not require the
WDK. Driver development requires the matching WDK and SDK build number.
