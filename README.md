# FitViber

GPS activity overlay video editor. Import Garmin .fit files, overlay activity data (speed, heart rate, cadence, power, elevation, GPS map) onto video clips, and export the result.

## Features

- Parse Garmin .fit files (speed, HR, cadence, power, elevation, GPS coordinates, laps)
- Video playback with frame-accurate seeking
- Configurable overlay panels with drag-to-position
- Timeline with multi-track view and FIT-video time sync
- Export with burn-in overlays or subtitle/data track
- Dark theme UI

## Requirements

- Windows 10/11
- Visual Studio 2022 Build Tools (MSVC)
- Qt 6.10.1 (MSVC 2022 64-bit) installed to `D:\Qt\6.10.1\msvc2022_64`
- CMake 3.16+
- Ninja build system

### Third-party Dependencies

Place these in the `thirdparty/` directory before building:

- **Garmin FIT SDK**: Download from [Garmin Developer](https://developer.garmin.com/fit/download/). Copy C++ SDK sources into `thirdparty/fit-sdk/cpp/`.
- **FFmpeg**: Download pre-built dev package (e.g., from [gyan.dev](https://www.gyan.dev/ffmpeg/builds/)). Place headers in `thirdparty/ffmpeg/include/`, import libraries in `thirdparty/ffmpeg/lib/`, and DLLs in `thirdparty/ffmpeg/bin/`.

## Building

```
build.bat
```

The executable will be at `bin/FitViber.exe` with all required DLLs deployed.

## Usage

1. Launch `bin/FitViber.exe`
2. Import a video file and a .fit file via the Media Browser
3. Drag the FIT track on the timeline to align with the video
4. Configure overlay panels in the Properties panel
5. Preview the result in real-time
6. Export the final video
