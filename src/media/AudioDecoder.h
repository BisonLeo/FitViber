#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>

struct AudioInfo {
    int sampleRate = 0;
    int channels = 0;
    int bitsPerSample = 0;
    double duration = 0.0;
    QString codecName;
};

class AudioDecoder : public QObject {
    Q_OBJECT
public:
    explicit AudioDecoder(QObject* parent = nullptr);
    ~AudioDecoder();

    bool open(const QString& filePath);
    void close();
    bool isOpen() const { return m_isOpen; }

    QByteArray decodeAll();
    bool seek(double seconds);
    const AudioInfo& info() const { return m_info; }

private:
    bool m_isOpen = false;
    AudioInfo m_info;
};
