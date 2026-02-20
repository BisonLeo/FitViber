#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include "Clip.h"

enum class TrackType {
    Video,
    Audio,
    FitData
};

class Track : public QObject {
    Q_OBJECT
public:
    explicit Track(TrackType type, const QString& name, QObject* parent = nullptr);
    ~Track();

    TrackType type() const { return m_type; }
    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    void addClip(const Clip& clip);
    void removeClip(int index);
    int clipCount() const { return static_cast<int>(m_clips.size()); }
    Clip& clip(int index) { return m_clips[index]; }
    const Clip& clip(int index) const { return m_clips[index]; }
    const std::vector<Clip>& clips() const { return m_clips; }
    std::vector<Clip>& clips() { return m_clips; }

    double duration() const;
    bool isMuted() const { return m_muted; }
    void setMuted(bool muted) { m_muted = muted; }

private:
    TrackType m_type;
    QString m_name;
    std::vector<Clip> m_clips;
    bool m_muted = false;
};
