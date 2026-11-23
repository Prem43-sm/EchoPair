# Build

## Requirements

- Windows 10 22H2 or newer, x64
- Visual Studio 2022 C++ build tools
- CMake 3.20 or newer

## Build and test

From a PowerShell prompt in the repository root:

```powershell
.\build.ps1
```

The script configures an x64 Visual Studio 2022 build, compiles the program,
and runs the registered CTest checks. To run tests separately:

```powershell
.\test.ps1
```

The executable is written to `build\Release\EchoPair.exe`.

The current integration test uses the Windows Core Audio API on the machine
where it runs. It checks endpoint metadata returned by Windows; it does not
test playback or physical Bluetooth routing.
