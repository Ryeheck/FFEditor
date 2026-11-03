#ifndef EVENTBUS_H
#define EVENTBUS_H

#include "player/Decoder.h"

#include <qobject.h>
#include <qtmetamacros.h>
#include <qtypes.h>
class EventBus : public QObject
{
    Q_OBJECT

private:
    EventBus() = default;
    std::atomic<float> m_volume{0.0f};

public:
    static EventBus& instance()
    {
        static EventBus bus;
        return bus;
    }

    float volume() const {  return m_volume.load();  };

    EventBus(const EventBus &) = delete;
    EventBus operator=(const EventBus &) = delete;

public slots:
    void setVolume(float value)
    {
        m_volume.store(value);
        
        emit volumeChanged(value);
    }

signals:
    void seekRequested(qint64 posMs);
    void positionChanged(qint64 posMs);
    void positionAudioChanged(qint64 posMs);
    void durationChanged(qint64 durationMs);
    void decoderFinished();
    void volumeChanged(const float &value);
    void frameChanged(const AVFramePtr &vFrame);
    void startPlayRequested(const QString &path);
};

#endif // EVENTBUS_H