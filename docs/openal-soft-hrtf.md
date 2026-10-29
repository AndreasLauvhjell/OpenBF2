# OpenAL Soft and HRTF audio

This guide describes an optional local audio setup for the rebuilt x86 client: OpenAL Soft as
the system OpenAL, with HRTF rendering for headphones. It is `[CHOSEN, not recovered]` policy for
testing, not retail behaviour. The build does not download or stage OpenAL Soft; every file
below is added by hand next to the rebuilt client.

## How the client picks an OpenAL

`OpenALProvider` (`Code/BF2/Sound/SoundEngine/OpenALProvider.cpp`) maps the Audio.con provider
and EAX setting to one of seven provider rows:

| Audio.con provider | EAX | Provider row | Library |
| --- | --- | --- | --- |
| `software` | on/off | Software Audio (+ EAX) | `BF2OpenAL.dll`, device `DirectSound` |
| `hardware` | on/off | Hardware Audio (+ EAX) | `BF2OpenAL.dll`, device `DirectSound3D` |
| `creative` | on/off | System OpenAL (+ EAX) | `OpenAL32.dll`, default device |

Only `creative` loads `OpenAL32.dll`. `LoadLibrary` searches the executable's directory first, so
a copy in `bin_client` takes precedence over a system-wide OpenAL. A system OpenAL is accepted only
when it reports the `EAX5.0` extension, unless `AudioSettings.setAlwaysAllowSystemOpenAL 1` is set.
`software` and `hardware` keep loading the retail `BF2OpenAL.dll` unchanged.

An empty or unknown provider name in Audio.con selects Software Audio when EAX is on (and fails
to apply when it is off), and the options menu then
shows the untranslated key `AUDIOSETTINGS_` because it localises `AUDIOSETTINGS_<provider>`.
All test clients that share a profile also share its Audio.con, so the client that saves last
decides the provider of the next start.

## Setup

1. Download the OpenAL Soft Windows binary package (`openal-soft-<version>-bin.zip`) from the
   OpenAL Soft releases. This setup was tested with 1.25.2.
2. Copy `bin\Win32\soft_oal.dll` to `out\build\x86\client\bin_client\OpenAL32.dll`. The client is
   32-bit; the `Win64` build does not load.
3. Put an `alsoft.ini` next to `BF2.exe`, or point `ALSOFT_CONF` at one:

   ```ini
   [general]
   stereo-encoding = hrtf
   stereo-mode = headphones
   ```

   `hrtf = true` still works but is deprecated in favour of `stereo-encoding`.
4. Select the Creative provider with EAX in the audio options and restart the client.

OpenAL Soft 1.25.2 reports `EAX5.0`, so it passes the provider check, and it emulates EAX 3.0,
`EAXSet`/`EAXGet` and EAX-RAM, so the client's reverb path stays active.

## Output device

The client opens the default device (`alcOpenDevice(NULL)`), so OpenAL Soft plays on the Windows
default playback device. Its WASAPI backend has no configuration option to choose another device.
Set the headphone output as the Windows default, or assign it to `BF2.exe` in the Windows volume
mixer.

## Diagnostics

Set `ALSOFT_LOGLEVEL=3` and `ALSOFT_LOGFILE=<path>` before starting the client. The log shows
the configuration files read, the device opened, the mix format and whether HRTF rendering was
enabled (for example `Full HRTF rendering enabled, using "Built-In HRTF"`). No log file means
`OpenAL32.dll` was never loaded, usually because another provider was selected.

## Direct channels for music and 2D sources

Under HRTF, OpenAL Soft renders a multi-channel buffer as virtual speakers at ±30 degrees through
the HRTF filters, which makes stereo music and stereo 2D samples sound further away and coloured.
`[CHOSEN, not recovered]`, not in retail (issue pending): when the loaded OpenAL reports
`AL_SOFT_direct_channels`, `OpenALProvider::open` sets `g_directChannelsAvailable` and traces
`AL_SOFT_direct_channels available: Yes`. Then:

- the music thread sets `AL_DIRECT_CHANNELS_SOFT` on its source;
- `OpenALSample::start` sets it on 2D samples and clears it on 3D samples, because sources are
  pooled.

Music and stereo 2D sources play straight to the output channels, while 3D sources keep full HRTF
spatialisation. The property has no effect on mono buffers. `BF2OpenAL.dll` and Creative's
OpenAL do not report the extension, so the retail provider paths never receive it.
