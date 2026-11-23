# Architecture

## Current implementation

The Windows application uses the documented Core Audio MMDevice API to
enumerate playback endpoints. It retrieves each endpoint's Windows-provided
identifier, friendly name, and state, and indicates the current console
default endpoint when Windows provides one.

No audio stream is opened or modified by this phase. The code does not classify
devices as Bluetooth based on their names.

## System-wide source routing

Endpoint enumeration is distinct from routing. WASAPI loopback capture can
read the mix rendered to an endpoint, but that alone does not transparently
replace Windows' system output or prevent duplicate/feedback paths when
rendering back to endpoints. A complete system-wide routing design must
resolve that source path before audio rendering is claimed.

No virtual endpoint or driver is included. The local environment has no
detected WDK installation, and driver requirements, packaging, and signing
must be evaluated before any driver work.
