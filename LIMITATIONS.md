# Current limitations

The application currently enumerates real Windows playback endpoints. It does
not yet capture system audio, render audio, synchronize endpoints, or install
an audio driver. Those capabilities are not simulated.

The architecture for system-wide audio routing is not considered settled.
Endpoint discovery alone does not provide a system-wide audio source or
multi-device playback.
