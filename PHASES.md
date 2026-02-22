full multi-phase plan for FitViber:                                
  ---                                                                             FitViber — Full Phase Plan
                                                                                  Phase 1: Project Skeleton (done)                                                                                                                                - Qt6/C++17 build system with CMake/Ninja/MSVC                                  - MainWindow with dock layout (MediaBrowser, PreviewWidget, TimelineWidget,     PropertiesPanel)                                                                - Dark theme UI, stub implementations for all modules                           - Conditional compilation: HAS_FIT_SDK, HAS_FFMPEG                                                                                                              Phase 2: FIT File Parsing (done)                                                                                                                                - Garmin FIT SDK integration                                                    - Parse .fit files into FitSession/FitTrack data
  - Time-indexed track with binary search + linear interpolation

  Phase 3: Video/Audio Decoding (done)

  - FFmpeg libav* wrapper (VideoDecoder, AudioDecoder)
  - Frame-accurate seeking, QImage output

  Phase 4: Media Browser, Hover Scrub, Preview Playback (done)

  - Grid media browser with thumbnails (video, image, FIT)
  - Hover scrub on video thumbnails
  - PreviewWidget with play/pause, seek slider, time display, step buttons
  - VideoPlaybackEngine with background decode thread + frame queue

  Phase 5: Drag-Drop, Absolute Timestamps, Playhead Seek (done)

  - Drag from MediaBrowser → drop onto TimelineWidget
  - Absolute wall-clock timeline (Unix timestamps)
  - Timestamp extraction: FFmpeg metadata → DJI filename → EXIF → file birthTime
  - Wall-clock ruler (HH:MM:SS), image clips = 5s default
  - Timeline click/scrub → seek video in PreviewWidget

  Phase 5b: Clip Selection, Snap, Move, Delete, Lock (done)

  - Single/multi-select clips (Ctrl+click)
  - Snap to clip edges on drop and drag-move
  - No-overlap enforcement
  - Drag-to-move (unlocked clips), Delete key, right-click menu (Delete /
  Lock-Unlock)
  - Ruler click = playhead scrub, track click = clip selection

  Phase 5c: Playback Sync (done)

  - Playhead updates during video playback

  Phase 5d: Use timeline for playback (upcoming)

  - According the arranged clips to play video in preview widget
  - Show only blank (black) if there is no clips at the current time marker bar
  - When the user moves the current time marker but not playing, the preview should update
  - When the user moves during playing, the preview should seek to that timestamp and continue

  Phase 6: Overlay Panels & Rendering (upcoming)

  - OverlayRenderer composites panels onto video frames
  - Concrete panels: Speed, HR, Cadence, Power, Elevation, MiniMap, Distance,
  Lap
  - Normalized coordinates (0.0–1.0) for resolution independence

  Phase 7: FIT ↔ Video Time Sync

  - TimeSync offset alignment between FIT track and video clips
  - Drag-to-sync in timeline UI

  Phase 8: Properties Panel

  - Panel configuration UI (position, size, style, data source)
  - Per-clip and per-panel properties editing

  Phase 9: Export Pipeline

  - MediaExporter with FFmpeg encoding
  - Burn-in overlay mode and subtitle/data track mode

  Phase 10: Export Dialog

  - Export settings UI (resolution, codec, bitrate, output path)
  - Progress display, final export execution
