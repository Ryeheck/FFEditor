#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include "Decoder.h"

#include <cstdint>
#include <qtypes.h>
#include <QObject>
#include <atomic>

extern "C" {
#include "miniaudio.h"
}

enum class AudioFormat {
    Stereo,
    Mono
};

class AudioOutput : public QObject
{
    Q_OBJECT
    
public:
    explicit AudioOutput(QObject *parent = nullptr);
    ~AudioOutput() override;
    
    
public slots:
    void start();
    bool init(Decoder *decoder, uint64_t samplesRate, AudioFormat format = AudioFormat::Stereo);
    void seekTo(qint64 posMs);
    qint64 getAudioClockMs();
    void stop();
    void setVolume(const float &volume);
    void setDuration(const uint64_t &duration);

private:
    static void audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
    void readSamples(float *pOutput, ma_uint32 frameCount);
    

    ma_device m_audioDevice;
    bool m_init = false;
    Decoder *m_decoder = nullptr;
    audioFrame m_aFrame;
    size_t m_aFrameOffset = 0;
    uint64_t m_sampleRate = 0;
    uint64_t m_duration   = 0;
    std::atomic<uint64_t> m_playedAudioSamples{0};
    std::atomic<int> m_audioFormat{2};
};

#endif // AUDIOOUTPUT_H