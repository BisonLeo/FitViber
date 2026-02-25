# FitViber - AI Agent Guide

> This file provides essential context for AI coding agents working on the FitViber project.

## Project Overview

**FitViber** is a Qt6/C++17 desktop application for creating GPS activity overlay videos. It reads Garmin .fit files, overlays configurable activity data panels (speed, heart rate, cadence, power, elevation, GPS map, etc.) onto video clips, and exports the result using FFmpeg.

### Key Technologies
- **Language**: C++17
- **GUI Framework**: Qt 6.10.1 (MSVC 2022 64-bit)
- **Build System**: CMake 3.16+ with Ninja
- **Media Processing**: FFmpeg (libavcodec, libavformat, libavutil, libswscale, libswresample)
- **Data Parsing**: Garmin FIT C++ SDK
- **Platform**: Windows 10/11

## Build Commands

### Standard Build (Interactive)
```cmd
build.bat
```

### CI / Non-interactive Build (for automation)
```cmd
build_ci.bat
```

### Manual Build (requires MSVC and Qt in PATH)
```bash
cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=D:/Qt/6.10.1/msvc2022_64 ..
ninja
```

### Run Application
```cmd
bin\FitViber.exe
```

### Run Tests
Test executables are auto-discovered from `test/test_*.cpp`. Each test file becomes its own executable:
```cmd
bin\test_fit_parser.exe
bin\test_media_decode.exe
bin\test_overlay_config.exe
bin\test_time_sync.exe
```

## Architecture & Source Layout

```
src/
├── app/           # Application entry point (main.cpp) and MainWindow
├── fit/           # Garmin FIT SDK wrappers (FitParser, FitTrack, FitData)
├── media/         # FFmpeg integration (VideoDecoder, AudioDecoder, MediaExporter, VideoPlaybackEngine)
├── overlay/       # Rendering engine (OverlayRenderer, OverlayPanel, OverlayConfig)
│   └── panels/    # Concrete overlay panels (Speed, HeartRate, Cadence, Power, Elevation, MiniMap, Distance, Lap, Inclination)
├── timeline/      # Multi-track timeline (TimelineModel, TimelineWidget, Track, Clip, TimeSync)
├── ui/            # UI widgets (MediaBrowser, PreviewWidget, PropertiesPanel, PlaybackController, DarkTheme)
└── util/          # Utility helpers (TimeUtil, ImageUtil)

test/              # Auto-discovered unit tests (test_*.cpp pattern)
thirdparty/        # External dependencies (fit-cpp-sdk as git submodule, ffmpeg pre-built)
testdata/          # Test data files (.fit, .mp4, .jpg)
bin/               # Build output with deployed DLLs (generated)
```

## Core Components

| Component | Location | Description |
|-----------|----------|-------------|
| MainWindow | `src/app/MainWindow` | QMainWindow with QDockWidget layout |
| FitParser | `src/fit/FitParser` | Wraps Garmin FIT SDK to decode .fit files |
| FitTrack | `src/fit/FitTrack` | Time-indexed FIT data with binary search + linear interpolation for O(log n) lookup |
| VideoDecoder | `src/media/VideoDecoder` | FFmpeg wrapper for decoding video frames to QImage |
| MediaExporter | `src/media/MediaExporter` | FFmpeg encoding pipeline with burn-in overlay support |
| OverlayRenderer | `src/overlay/OverlayRenderer` | Composites overlay panels onto QImage frames via QPainter |
| OverlayPanel | `src/overlay/OverlayPanel` | Abstract base class for overlay data panels |
| TimelineWidget | `src/timeline/TimelineWidget` | Custom-painted multi-track timeline with ruler, playhead, zoom |
| TimeSync | `src/timeline/TimeSync` | Manages FIT-video timestamp offset alignment |
| PreviewWidget | `src/ui/PreviewWidget` | Video frame display with playback controls |

## Data Flow

```
.fit file -> FitParser -> FitSession/FitTrack
                                |
                                v
Video file -> VideoDecoder -> QImage frames -> OverlayRenderer -> PreviewWidget
                                                    ^
                                              TimeSync (offset)
                                                    ^
                                            TimelineWidget (user drags to align)
```

## Development Conventions

### Code Style
- Use `#pragma once` for header guards
- Use `camelCase` for functions and variables, `PascalCase` for classes
- Prefer `inline constexpr` for constants in namespaces
- Use Qt's container classes (`QString`, `QVector`, etc.) for Qt-related code
- Use standard C++ containers for non-Qt logic

### Conditional Compilation
The codebase uses preprocessor macros to handle optional dependencies:
- `HAS_FIT_SDK` - Defined when Garmin FIT SDK is available
- `HAS_FFMPEG` - Defined when FFmpeg libraries are available

Always guard real SDK/FFmpeg code with these macros to avoid breaking builds where dependencies are absent:

```cpp
#ifdef HAS_FIT_SDK
    // Real FIT parsing code
#else
    // Stub implementation
#endif
```

### Qt-Specific Conventions
- `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, `CMAKE_AUTOUIC` are enabled
- Use `Q_OBJECT` macro in all QObject-derived classes
- Use normalized coordinates (0.0-1.0) for overlay panels to maintain resolution independence
- Apply `DarkTheme` at application level; avoid hardcoding colors in widget constructors

### Overlay Panels
- All panels inherit from `OverlayPanel` base class
- Use normalized coordinates (0.0-1.0) for position and size
- The `resolveRect()` method converts normalized coordinates to pixel coordinates based on video resolution

### FIT Data
- FIT timestamps use FIT epoch (1989-12-31 00:00:00 UTC = Unix epoch + 631065600 seconds)
- `FitTrack` provides O(log n) lookup with binary search + linear interpolation
- GPS coordinates are stored in decimal degrees (latitude: -90 to 90, longitude: -180 to 180)

## Testing Strategy

- Tests are located in `test/` directory
- Each `test_*.cpp` file is compiled into a standalone executable
- Tests use simple `assert()` statements and `printf()` for output
- Tests are copied to `bin/` directory to resolve DLL dependencies
- Tests can be run individually or as part of CI pipeline

## Third-Party Dependencies

### Required Setup
Place these in `thirdparty/` before building:

1. **Garmin FIT SDK** (git submodule at `thirdparty/fit-cpp-sdk/`)
   - Repository: https://github.com/garmin/fit-cpp-sdk.git

2. **FFmpeg** (pre-built dev package at `thirdparty/ffmpeg_build_8_0_1/`)
   - Download from: https://www.gyan.dev/ffmpeg/builds/
   - Expected structure: `include/`, `lib/`, `bin/`

### Git Submodules
```bash
git submodule update --init --recursive
```

## Hardcoded Paths

The following paths are hardcoded in the build system:
- Qt 6.10.1: `D:/Qt/6.10.1/msvc2022_64`
- MSVC BuildTools: `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat`

## Thread Model

- **Main thread**: UI operations and overlay rendering
- **Decode thread**: FFmpeg video/audio decoding (VideoPlaybackEngine)
- **Export thread**: FFmpeg encoding pipeline (MediaExporter)

## Common Tasks

### Adding a New Overlay Panel
1. Create header/implementation in `src/overlay/panels/`
2. Inherit from `OverlayPanel` base class
3. Implement `paint()`, `defaultLabel()`, and `panelType()` methods
4. Register in `OverlayPanelFactory`
5. Add to `PanelType` enum

### Adding a New Test
1. Create `test/test_<feature>.cpp`
2. Include necessary headers from `src/`
3. Use `assert()` for validation, `printf()` for output
4. Re-run CMake to auto-discover the new test

### Working with FIT Data
```cpp
// Parse a .fit file
FitParser parser;
if (parser.parse("path/to/file.fit")) {
    const FitSession& session = parser.session();
    // Access session.records, session.laps, etc.
}

// Create time-indexed track
FitTrack track;
track.loadSession(session);
FitRecord record = track.getRecordAtTime(timestamp);
```

## Security Considerations

- File paths from user input should be validated before use
- FFmpeg operations run in separate threads to prevent UI blocking
- No sensitive data should be logged to console

## Resources

- **Test data**: `testdata/` contains sample .fit files, videos, and images
- **Documentation**: See `CLAUDE.md`, `GEMINI.md`, `README.md`, `PHASES.md` for additional context
- **Phase plan**: `PHASES.md` contains the development roadmap and current status
