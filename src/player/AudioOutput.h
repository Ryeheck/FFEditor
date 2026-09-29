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
    
    
public slots:
    void start();
    bool init(Decoder *decoder, uint64_t samplesRate);
    void seekTo(qint64 posMs);
    qint64 getAudioClockMs();
    void stop();
    
private:
    static void audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
    void readSamples(float *pOutput, ma_uint32 frameCount);
    

    ma_device m_audioDevice;
    bool m_init = false;
    Decoder *m_decoder = nullptr;
    audioFrame m_aFrame;
    size_t m_aFrameOffset;
    uint64_t m_sampleRate;
    std::atomic<uint64_t> m_playedAudioSamples{0};
};

#endif // AUDIOOUTPUT_H