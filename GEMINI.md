# FitViber - Developer Guide

## Project Overview

**FitViber** is a C++17 desktop application built with Qt 6.10.1. It functions as a GPS activity overlay video editor. The core objective of the application is to read Garmin `.fit` files, synchronize the activity data (speed, heart rate, cadence, power, elevation, etc.) with video clips, overlay customizable data panels onto the video, and export the composited result using FFmpeg.

### Key Technologies
*   **Language:** C++17
*   **GUI Framework:** Qt 6.10.1 (MSVC 2022 64-bit)
*   **Build System:** CMake (3.16+) and Ninja
*   **Media Processing:** FFmpeg (libavcodec, libavformat, libavutil, libswscale, libswresample)
*   **Data Parsing:** Garmin FIT C++ SDK

### Architecture & Source Layout

The source code is modularized within the `src/` directory:

*   **`src/app/`**: Application entry point (`main.cpp`) and the primary layout (`MainWindow`).
*   **`src/fit/`**: Wrappers for the Garmin FIT SDK to decode sessions and time-indexed tracks (`FitParser`, `FitTrack`).
*   **`src/media/`**: FFmpeg integration for decoding frames (`VideoDecoder`, `AudioDecoder`) and the export pipeline (`MediaExporter`).
*   **`src/overlay/`**: The rendering engine (`OverlayRenderer`) and base/concrete classes for various overlay panels (Speed, HeartRate, Cadence, MiniMap, etc.).
*   **`src/timeline/`**: UI and models for a multi-track timeline, including drag-to-sync capabilities to align video absolute timestamps with FIT epoch timestamps.
*   **`src/ui/`**: Core application widgets like the `MediaBrowser`, `PreviewWidget`, `PropertiesPanel`, and the `DarkTheme` implementation.
*   **`src/util/`**: Utility functions for time conversion and image manipulation.
*   **`test/`**: Auto-discovered unit test executables.
*   **`thirdparty/`**: External dependencies (FFmpeg pre-builds and Garmin FIT SDK).

## Building and Running

### Prerequisites
*   Windows 10/11
*   Visual Studio 2022 Build Tools (MSVC)
*   Qt 6.10.1 installed to `D:\Qt\6.10.1\msvc2022_64`
*   FFmpeg dev package & Garmin FIT SDK populated in the `thirdparty/` directory.

### Build Commands

*   **Standard Build (Interactive):**
    ```cmd
    build.bat
    ```
*   **CI / Non-interactive Build:**
    ```bash
    ./build_ci.bat
    ```
*   **Manual Build (if environment is already set up):**
    ```bash
    cd build
    cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="D:/Qt/6.10.1/msvc2022_64" ..
    ninja
    ```

### Running the Application
The build scripts will automatically deploy the executable and required Qt/FFmpeg DLLs into the `bin/` directory.

```cmd
bin\FitViber.exe
```

### Running Tests
Test files are located in `test/` (e.g., `test_fit_parser.cpp`). CMake automatically discovers files matching `test_*.cpp` and compiles them into standalone executables. They are copied to the `bin/` directory to easily resolve DLL dependencies.

```cmd
bin\test_fit_parser.exe
bin\test_media_decode.exe
bin\test_overlay_config.exe
bin\test_time_sync.exe
```

## Development Conventions

*   **Conditional Compilation:** The codebase heavily relies on preprocessor macros `HAS_FIT_SDK` and `HAS_FFMPEG`. When working on related modules, ensure these stubs/conditions are appropriately respected to avoid breaking builds where dependencies are absent.
*   **Coordinate System:** Overlay panels (`src/overlay/panels/`) utilize **normalized coordinates** (ranging from 0.0 to 1.0) rather than absolute pixels. This allows the overlays to remain resolution-independent when switching between preview scaling and final export resolution.
*   **Qt Metadata:** `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, and `CMAKE_AUTOUIC` are enabled. Qt macros like `Q_OBJECT`, signals, and slots should be used conventionally.
*   **UI Styling:** The application uses a programmatic `DarkTheme` applied at the application level in `main.cpp`. Avoid hardcoding colors directly in widget constructors; rely on Qt palettes or the theme definitions.
*   **Testing Protocol:** When adding a new module or fixing a core bug, update or add a new test case in the `test/` directory. Since each `test_*.cpp` is an isolated executable, include simple `assert()` statements and standard output for verification.