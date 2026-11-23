# Testing

Run the build and automated tests from PowerShell:

```powershell
.\build.ps1
```

The endpoint integration test calls Windows Core Audio and validates that
returned endpoint identifiers are non-empty and unique and that each endpoint
has a recognized state. It runs against the actual Windows machine; it does
not use a fake device list. A machine with no playback endpoints may return an
empty list.

Playback, multiple-device routing, disconnect recovery, synchronization, and
Bluetooth hardware tests are not implemented yet.
