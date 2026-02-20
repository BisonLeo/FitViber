# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project Overview

FitViber is a Qt6/C++17 desktop application for creating GPS activity overlay videos. It reads Garmin .fit files, overlays configurable activity data panels (speed, HR, cadence, power, elevation, GPS map, etc.) onto video clips, and exports the result using FFmpeg.

## Build Commands

**Full build** (sets up MSVC environment, configures CMake with Ninja, builds, and deploys Qt DLLs to `bin/`):
```
build.bat
```

**Manual build from bash** (requires MSVC and Qt in PATH):
```
cd build && cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=D:/Qt/6.10.1/msvc2022_64 .. && ninja
```

**Run the application**: `bin/FitViber.exe`

**Tests**: Test executables are auto-discovered from `test/test_*.cpp`. Each test file becomes its own executable. After building, run individual tests from `build/test_<name>.exe`.

## Architecture

### Source Layout
- `src/` — Application source files (C++ and some C)
- `src/app/` — MainWindow and app constants
- `src/fit/` — Garmin FIT SDK wrapper, data structs, time-indexed track
- `src/media/` — FFmpeg video/audio decode/encode, frame queue, image conversion
- `src/overlay/` — Overlay rendering engine and panel base class
- `src/overlay/panels/` — Concrete overlay panels (speed, HR, cadence, power, elevation, map, distance, lap)
- `src/timeline/` — Timeline model, widget, tracks, clips, FIT↔video time sync
- `src/ui/` — UI widgets (media browser, preview, properties, playback controller, dark theme)
- `src/util/` — Utility helpers (time conversion, image conversion)
- `thirdparty/fit-sdk/` — Garmin FIT C++ SDK (user provides)
- `thirdparty/ffmpeg/` — Pre-built FFmpeg dev package (user provides)
- `test/` — Unit tests (auto-discovered by CMake glob)
- `bin/` — Build output with deployed Qt/FFmpeg DLLs (generated)

### Core Components

**MainWindow** (`src/app/MainWindow`) — QMainWindow with QDockWidget layout: MediaBrowser (top-left), PreviewWidget (center), PropertiesPanel (right), TimelineWidget (bottom).

**FitParser** (`src/fit/FitParser`) — Wraps Garmin FIT C++ SDK to decode .fit files into FitSession/FitTrack data.

**FitTrack** (`src/fit/FitTrack`) — Time-indexed FIT data with binary search + linear interpolation for O(log n) lookup.

**VideoDecoder** (`src/media/VideoDecoder`) — FFmpeg libav* wrapper for decoding video frames to QImage.

**MediaExporter** (`src/media/MediaExporter`) — FFmpeg encoding pipeline supporting burn-in overlay and subtitle/data track export modes.

**OverlayRenderer** (`src/overlay/OverlayRenderer`) — Composites overlay panels onto QImage frames via QPainter.

**OverlayPanel** (`src/overlay/OverlayPanel`) — Abstract base class for overlay data panels. Panels use normalized coordinates (0.0-1.0) for resolution independence.

**TimelineWidget** (`src/timeline/TimelineWidget`) — Custom-painted multi-track timeline with ruler, playhead, zoom, and drag-to-sync.

**TimeSync** (`src/timeline/TimeSync`) — Manages FIT-video timestamp offset alignment.

**PreviewWidget** (`src/ui/PreviewWidget`) — Video frame display with playback controls.

**PlaybackController** (`src/ui/PlaybackController`) — Play/pause/seek state machine driven by QTimer at video FPS.

### Data Flow

```
.fit file -> FitParser -> FitSession/FitTrack
Video file -> VideoDecoder -> QImage frames -> OverlayRenderer -> PreviewWidget
                                                    ^
                                              TimeSync (offset)
                                                    ^
                                            TimelineWidget (user drags to align)
```

### Key Technical Details

- **Qt MOC**: `CMAKE_AUTOMOC` is enabled
- **Conditional compilation**: `HAS_FIT_SDK` and `HAS_FFMPEG` preprocessor defines control whether real SDK/FFmpeg code or stubs are compiled
- **Qt path is hardcoded** in CMakeLists.txt to `D:/Qt/6.10.1/msvc2022_64`
- **WIN32_EXECUTABLE ON** suppresses the console window
- **Thread model**: Main thread for UI/overlay rendering, decode thread for FFmpeg, export thread for encoding pipeline
- **Overlay panels use normalized coordinates** (0.0-1.0) relative to video resolution
