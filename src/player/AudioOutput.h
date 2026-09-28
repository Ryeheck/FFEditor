#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include "Decoder.h"

#include <qtypes.h>
#include <QObject>
#include <atomic>

extern "C" {
#include "miniaudio.h"
}


class AudioOutput : public QObject
{
    Q_OBJECT

signals:
    
    
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput() override;
    bool initAudio();
    
public slots:
   

private:
    static void audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
    void processNextAFrame(float *pOutput, ma_uint32 frameCount);
    qint64 getAudioClockMs();

    ma_device m_audioDevice;
    bool m_audioInit        = false;
    Decoder *m_decoder = nullptr;
    audioFrame m_aFrame;
    size_t m_aFrameOffset;
    std::atomic<uint64_t> m_playedAudioSamples{0};
};

#endif // AUDIOOUTPUT_H