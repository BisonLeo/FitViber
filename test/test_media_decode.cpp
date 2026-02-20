#include <cassert>
#include <cstdio>
#include <cmath>
#include "media/MediaProbe.h"
#include "media/VideoDecoder.h"
#include "media/AudioDecoder.h"

static const char* TEST_VIDEO = "../testdata/DJI_20260210140425_0011_D.mp4";

void test_media_probe() {
    printf("=== test_media_probe ===\n");

#ifdef HAS_FFMPEG
    MediaProbe probe;
    bool ok = probe.probe(TEST_VIDEO);

    if (!ok) {
        printf("FAIL: probe failed - %s\n", probe.errorString().toUtf8().constData());
        assert(false);
    }

    const MediaInfo& info = probe.info();

    printf("  Container: %s\n", info.containerFormat.toUtf8().constData());
    printf("  Duration: %.3f s\n", info.duration);
    printf("  Has Video: %s\n", info.hasVideo ? "yes" : "no");
    printf("  Has Audio: %s\n", info.hasAudio ? "yes" : "no");

    if (info.hasVideo) {
        printf("  Video: %dx%d @ %.2f fps\n", info.videoWidth, info.videoHeight, info.videoFps);
        printf("  Video Codec: %s\n", info.videoCodec.toUtf8().constData());
        printf("  Video Pixel Format: %s\n", info.videoPixelFormat.toUtf8().constData());
        printf("  Video Bitrate: %d bps\n", info.videoBitRate);
    }

    if (info.hasAudio) {
        printf("  Audio: %d Hz, %d ch\n", info.audioSampleRate, info.audioChannels);
        printf("  Audio Codec: %s\n", info.audioCodec.toUtf8().constData());
        printf("  Audio Bitrate: %d bps\n", info.audioBitRate);
    }

    // Creation time for FIT alignment
    printf("  Creation Time: %s\n", info.creationTimeStr.toUtf8().constData());
    printf("  Creation Timestamp (Unix): %.0f\n", info.creationTimestamp);

    // Print all metadata
    printf("  --- All Metadata ---\n");
    for (auto it = info.metadata.constBegin(); it != info.metadata.constEnd(); ++it) {
        printf("    %s = %s\n", it.key().toUtf8().constData(), it.value().toUtf8().constData());
    }

    assert(info.hasVideo);
    assert(info.videoWidth > 0);
    assert(info.videoHeight > 0);
    assert(info.videoFps > 0);
    assert(info.duration > 0);

    printf("PASS: test_media_probe\n\n");
#else
    printf("SKIP: test_media_probe (no FFmpeg)\n\n");
#endif
}

void test_video_decode_10_frames() {
    printf("=== test_video_decode_10_frames ===\n");

#ifdef HAS_FFMPEG
    VideoDecoder decoder;
    bool ok = decoder.open(TEST_VIDEO);

    if (!ok) {
        printf("FAIL: cannot open video\n");
        assert(false);
    }

    const VideoInfo& info = decoder.info();
    printf("  Video: %dx%d, %.2f fps, duration %.3f s, codec %s\n",
           info.width, info.height, info.fps, info.duration,
           info.codecName.toUtf8().constData());
    printf("  Total frames (estimated): %lld\n", static_cast<long long>(info.totalFrames));

    // Decode 10 frames
    for (int i = 0; i < 10; ++i) {
        QImage frame = decoder.decodeNextFrame();
        if (frame.isNull()) {
            printf("  Frame %d: NULL (end of stream or error)\n", i);
            break;
        }
        printf("  Frame %d: %dx%d, PTS=%.4f s\n",
               i, frame.width(), frame.height(), decoder.currentTime());
        assert(frame.width() == info.width);
        assert(frame.height() == info.height);
    }

    // Test seeking to middle
    double seekTarget = info.duration / 2.0;
    printf("  Seeking to %.3f s...\n", seekTarget);
    ok = decoder.seek(seekTarget);
    assert(ok);

    QImage seekFrame = decoder.decodeNextFrame();
    if (!seekFrame.isNull()) {
        printf("  After seek: PTS=%.4f s, %dx%d\n",
               decoder.currentTime(), seekFrame.width(), seekFrame.height());
    }

    decoder.close();
    assert(!decoder.isOpen());

    printf("PASS: test_video_decode_10_frames\n\n");
#else
    printf("SKIP: test_video_decode_10_frames (no FFmpeg)\n\n");
#endif
}

void test_audio_decode() {
    printf("=== test_audio_decode ===\n");

#ifdef HAS_FFMPEG
    AudioDecoder decoder;
    bool ok = decoder.open(TEST_VIDEO);

    if (!ok) {
        printf("  No audio stream found or cannot open - SKIP\n\n");
        return;
    }

    const AudioInfo& info = decoder.info();
    printf("  Audio: %d Hz, %d ch, codec %s, duration %.3f s\n",
           info.sampleRate, info.channels,
           info.codecName.toUtf8().constData(), info.duration);

    // Decode 0.1 seconds of audio
    QByteArray pcm = decoder.decode(0.1);
    int expectedBytes = static_cast<int>(0.1 * info.sampleRate * info.channels * 2); // S16

    printf("  Decoded %lld bytes of PCM (expected ~%d bytes for 0.1s)\n",
           static_cast<long long>(pcm.size()), expectedBytes);

    if (pcm.size() > 0) {
        // Check for non-silence
        const int16_t* samples = reinterpret_cast<const int16_t*>(pcm.constData());
        int numSamples = pcm.size() / 2;
        int nonZero = 0;
        int16_t maxVal = 0;
        for (int i = 0; i < numSamples; ++i) {
            if (samples[i] != 0) nonZero++;
            if (std::abs(samples[i]) > maxVal) maxVal = std::abs(samples[i]);
        }
        printf("  Non-zero samples: %d / %d, peak: %d\n", nonZero, numSamples, maxVal);
    }

    assert(pcm.size() > 0);

    decoder.close();
    printf("PASS: test_audio_decode\n\n");
#else
    printf("SKIP: test_audio_decode (no FFmpeg)\n\n");
#endif
}

int main() {
    test_media_probe();
    test_video_decode_10_frames();
    test_audio_decode();
    printf("All media decode tests passed.\n");
    return 0;
}
