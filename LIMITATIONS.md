# Current limitations

The application currently enumerates real Windows playback endpoints and can
play an explicitly requested test tone through one or two active endpoints.
It can watch Windows endpoint-change notifications, but active streams are not
automatically recovered after a device disconnect. It does not capture system
audio, synchronize endpoints, or install an audio driver. Those capabilities
are not simulated.

The architecture for system-wide audio routing is not considered settled.
Endpoint discovery alone does not provide a system-wide audio source or
multi-device playback.
