# Architecture

## Current implementation

The Windows application uses the documented Core Audio MMDevice API to
enumerate playback endpoints. It retrieves each endpoint's Windows-provided
identifier, friendly name, and state, and indicates the current console
default endpoint when Windows provides one.

The application can open shared-mode WASAPI render streams to up to two
active endpoints and play an explicitly requested 440 Hz test tone. Each
endpoint has an independent event-driven buffer, a preallocated SPSC PCM ring,
and a render thread. Tone generation runs separately from the render thread.
An endpoint notification watcher can detect Windows endpoint state, add/remove,
and default-device changes; the CLI's `--watch` mode re-enumerates after each
notification. This diagnostic path is not system-audio capture or a
synchronized multi-device route. The code does not classify devices as
Bluetooth based on their names.

## System-wide source routing

Endpoint enumeration is distinct from routing. WASAPI loopback capture can
read the mix rendered to an endpoint, but that alone does not transparently
replace Windows' system output or prevent duplicate/feedback paths when
rendering back to endpoints. A complete system-wide routing design must
resolve that source path before audio rendering is claimed.

System-wide routing will require a virtual render endpoint so Windows can
deliver its shared system mix to EchoPair without routing the source back into
the physical outputs. The Microsoft SysVAD sample documents a supported WDM
audio-driver starting point; its sample driver is not itself an EchoPair
fan-out engine. A custom device/driver path, package signing, and deployment
remain separate work. The WDK is installed for evaluation and build work only.
Visual Studio driver-toolset integration remains to be verified.
Secure Boot and driver-signing settings have not been changed.
