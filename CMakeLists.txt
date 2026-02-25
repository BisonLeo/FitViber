cmake_minimum_required(VERSION 3.16)
project(FitViber VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/utf-8)
    add_compile_definitions(UNICODE _UNICODE)
endif()

# Set Qt6 installation path
set(Qt6_DIR "D:/Qt/6.10.1/msvc2022_64/lib/cmake/Qt6")

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Multimedia
)

# FIT C++ SDK (git submodule in thirdparty/fit-cpp-sdk)
set(FIT_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fit-cpp-sdk)
if(EXISTS ${FIT_SDK_DIR}/CMakeLists.txt)
    add_subdirectory(${FIT_SDK_DIR})
    set(HAS_FIT_SDK TRUE)
    message(STATUS "Found FIT C++ SDK in thirdparty/fit-cpp-sdk/")
else()
    message(WARNING "FIT SDK not found - FIT parsing will use stub")
    set(HAS_FIT_SDK FALSE)
endif()

# FFmpeg pre-built libraries
# FFmpeg source is in thirdparty/ffmpeg (git submodule) but must be built
# separately. Set FFMPEG_ROOT to point to a pre-built FFmpeg installation,
# or build it from thirdparty/ffmpeg and install to thirdparty/ffmpeg-build.
if(NOT DEFINED FFMPEG_ROOT)
    # Check common locations
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ffmpeg_build_8_0_1)
        set(FFMPEG_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/ffmpeg_build_8_0_1)
    elseif(DEFINED ENV{FFMPEG_ROOT})
        set(FFMPEG_ROOT $ENV{FFMPEG_ROOT})
    endif()
endif()

set(FFMPEG_LIBRARIES "")
set(HAS_FFMPEG FALSE)

if(DEFINED FFMPEG_ROOT)
    set(FFMPEG_INCLUDE_DIR ${FFMPEG_ROOT}/include)
    set(FFMPEG_LIB_DIR ${FFMPEG_ROOT}/lib)
    set(FFMPEG_BIN_DIR ${FFMPEG_ROOT}/bin)

    foreach(LIB avcodec avformat avutil swscale swresample)
        find_library(FFMPEG_${LIB}_LIBRARY
            NAMES ${LIB}
            PATHS ${FFMPEG_LIB_DIR}
            NO_DEFAULT_PATH
        )
        if(FFMPEG_${LIB}_LIBRARY)
            list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${LIB}_LIBRARY})
            set(HAS_FFMPEG TRUE)
        endif()
    endforeach()
endif()

if(HAS_FFMPEG)
    message(STATUS "Found FFmpeg libraries: ${FFMPEG_LIBRARIES}")
else()
    message(WARNING "FFmpeg not found - set FFMPEG_ROOT or build thirdparty/ffmpeg to thirdparty/ffmpeg-build. Video features will use stubs.")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fit
    ${CMAKE_CURRENT_SOURCE_DIR}/src/media
    ${CMAKE_CURRENT_SOURCE_DIR}/src/overlay
    ${CMAKE_CURRENT_SOURCE_DIR}/src/overlay/panels
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timeline
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util
)

if(HAS_FFMPEG AND DEFINED FFMPEG_INCLUDE_DIR)
    include_directories(${FFMPEG_INCLUDE_DIR})
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/app/MainWindow.cpp
    src/app/ProjectManager.cpp
    src/fit/FitParser.cpp
    src/fit/FitTrack.cpp
    src/media/VideoDecoder.cpp
    src/media/VideoPlaybackEngine.cpp
    src/media/AudioDecoder.cpp
    src/media/MediaProbe.cpp
    src/media/MediaExporter.cpp
    src/media/ImageUtil.cpp
    src/overlay/OverlayRenderer.cpp
    src/overlay/OverlayPanel.cpp
    src/overlay/OverlayConfig.cpp
    src/overlay/OverlayPanelFactory.cpp
    src/overlay/panels/SpeedPanel.cpp
    src/overlay/panels/HeartRatePanel.cpp
    src/overlay/panels/CadencePanel.cpp
    src/overlay/panels/PowerPanel.cpp
    src/overlay/panels/ElevationPanel.cpp
    src/overlay/panels/MiniMapPanel.cpp
    src/overlay/panels/DistancePanel.cpp
    src/overlay/panels/LapPanel.cpp
    src/overlay/panels/InclinationPanel.cpp
    src/timeline/TimelineModel.cpp
    src/timeline/TimelineWidget.cpp
    src/timeline/Track.cpp
    src/timeline/TimeSync.cpp
    src/ui/MediaBrowser.cpp
    src/ui/PreviewWidget.cpp
    src/ui/PreviewCanvas.cpp
    src/ui/PropertiesPanel.cpp
    src/ui/PlaybackController.cpp
    src/ui/DarkTheme.cpp
)

set(HEADERS
    src/app/MainWindow.h
    src/app/AppConstants.h
    src/app/ProjectManager.h
    src/fit/FitParser.h
    src/fit/FitData.h
    src/fit/FitTrack.h
    src/media/VideoDecoder.h
    src/media/VideoPlaybackEngine.h
    src/media/AudioDecoder.h
    src/media/MediaProbe.h
    src/media/FrameQueue.h
    src/media/MediaExporter.h
    src/media/ImageUtil.h
    src/overlay/OverlayRenderer.h
    src/overlay/OverlayPanel.h
    src/overlay/OverlayConfig.h
    src/overlay/OverlayPanelFactory.h
    src/overlay/panels/SpeedPanel.h
    src/overlay/panels/HeartRatePanel.h
    src/overlay/panels/CadencePanel.h
    src/overlay/panels/PowerPanel.h
    src/overlay/panels/ElevationPanel.h
    src/overlay/panels/MiniMapPanel.h
    src/overlay/panels/DistancePanel.h
    src/overlay/panels/LapPanel.h
    src/overlay/panels/InclinationPanel.h
    src/timeline/TimelineModel.h
    src/timeline/TimelineWidget.h
    src/timeline/Track.h
    src/timeline/Clip.h
    src/timeline/ClipTransform.h
    src/timeline/TimeSync.h
    src/ui/MediaBrowser.h
    src/ui/PreviewWidget.h
    src/ui/PreviewCanvas.h
    src/ui/PropertiesPanel.h
    src/ui/PlaybackController.h
    src/ui/DarkTheme.h
    src/util/TimeUtil.h
    src/util/ImageUtil.h
)

# Create executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
)

# Compile definitions
if(HAS_FIT_SDK)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_FIT_SDK)
endif()
if(HAS_FFMPEG)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_FFMPEG)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
)

if(HAS_FIT_SDK)
    target_link_libraries(${PROJECT_NAME} fit_sdk)
endif()

if(HAS_FFMPEG)
    target_link_libraries(${PROJECT_NAME} ${FFMPEG_LIBRARIES})
endif()

# Windows specific settings
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )

    # Deploy Qt DLLs automatically using windeployqt
    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # Run windeployqt after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/bin
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_LIST_DIR}/bin/
        COMMAND ${WINDEPLOYQT_EXECUTABLE} --release --no-translations ${CMAKE_CURRENT_LIST_DIR}/bin/$<TARGET_FILE_NAME:${PROJECT_NAME}>
        COMMENT "Deploying Qt DLLs to bin directory..."
    )

    # Copy FFmpeg DLLs to bin/
    if(HAS_FFMPEG AND EXISTS ${FFMPEG_BIN_DIR})
        file(GLOB FFMPEG_DLLS "${FFMPEG_BIN_DIR}/*.dll")
        foreach(DLL ${FFMPEG_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${CMAKE_CURRENT_LIST_DIR}/bin/
            )
        endforeach()
    endif()
endif()

# Installation
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_CURRENT_LIST_DIR}/bin
)

# =============================================================================
# Test Targets
# =============================================================================

# Common test helper sources needed by tests
set(TEST_HELPER_SOURCES
    src/fit/FitParser.cpp
    src/fit/FitTrack.cpp
    src/overlay/OverlayPanel.cpp
    src/overlay/OverlayConfig.cpp
    src/media/MediaProbe.cpp
    src/media/VideoDecoder.cpp
    src/media/AudioDecoder.cpp
    src/media/ImageUtil.cpp
)

file(GLOB TEST_SOURCES "test/test_*.cpp")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${TEST_HELPER_SOURCES})
    target_link_libraries(${TEST_NAME} Qt6::Core Qt6::Gui Qt6::Widgets)
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fit
        ${CMAKE_CURRENT_SOURCE_DIR}/src/media
        ${CMAKE_CURRENT_SOURCE_DIR}/src/overlay
        ${CMAKE_CURRENT_SOURCE_DIR}/src/util
    )
    if(HAS_FIT_SDK)
        target_link_libraries(${TEST_NAME} fit_sdk)
        target_compile_definitions(${TEST_NAME} PRIVATE HAS_FIT_SDK)
    endif()
    if(HAS_FFMPEG)
        target_link_libraries(${TEST_NAME} ${FFMPEG_LIBRARIES})
        target_compile_definitions(${TEST_NAME} PRIVATE HAS_FFMPEG)
        target_include_directories(${TEST_NAME} PRIVATE ${FFMPEG_INCLUDE_DIR})
    endif()

    # Copy test executables to bin/ so they can find Qt DLLs
    add_custom_command(TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${TEST_NAME}> ${CMAKE_CURRENT_LIST_DIR}/bin/
    )
endforeach()

list(LENGTH TEST_SOURCES TEST_COUNT)
message(STATUS "Found ${TEST_COUNT} test(s) in test/ directory")
