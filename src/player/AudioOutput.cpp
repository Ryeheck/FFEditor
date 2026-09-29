#define MINIAUDIO_IMPLEMENTATION

#include "AudioOutput.h"

#include <atomic>

extern "C" {
#include "miniaudio.h"
}

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    
}

AudioOutput::~AudioOutput()
{

}

bool AudioOutput::init(Decoder *decoder, uint64_t sampleRate)
{
    m_decoder = decoder;
    m_sampleRate = sampleRate;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; // Works with float32 ([-1 : 1])
    config.playback.channels = 2;             // Stereo
    config.sampleRate        = m_sampleRate;         // Ghz
    config.dataCallback      = audioCallback; // Function
    config.pUserData         = this;          // Ptr on this player

    if (ma_device_init(NULL, &config, &m_audioDevice) != MA_SUCCESS)
        return false;

    m_aFrameOffset = 0;
    m_playedAudioSamples.store(0);
    m_aFrame = audioFrame{};
    m_init = true;

    return true;
}

void AudioOutput::audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    AudioOutput *player = static_cast<AudioOutput *>(pDevice->pUserData);

    if (player && m_init)
        player->readSamples(static_cast<float *>(pOutput), frameCount);
}

void AudioOutput::readSamples(float *pOutput, ma_uint32 frameCount)
{
    // STEREO
    size_t samplesNeeded = frameCount * 2;
    size_t samplesFilled = 0;

    while (samplesFilled < samplesNeeded) 
    {
        if (m_aFrameOffset >= m_aFrame.samples.size()) {
            audioFrame nextFrame;

            if (m_decoder && m_decoder->getNextAFrame(nextFrame)) {
                m_aFrame = std::move(nextFrame);
                m_aFrameOffset = 0;
            } else {
                std::fill_n(pOutput + samplesFilled, samplesNeeded - samplesFilled, 0.0f);
                break;
            }
        }
        size_t samplesAvailable = m_aFrame.samples.size() - m_aFrameOffset;
        size_t samplesToCopy = std::min(samplesNeeded -  samplesFilled, samplesAvailable);
        std::copy_n(m_aFrame.samples.data() + m_aFrameOffset,
                    samplesToCopy,
                    pOutput + samplesFilled);
        
        m_aFrameOffset += samplesToCopy;
        samplesFilled += samplesToCopy;

    }
    m_playedAudioSamples.fetch_add(frameCount);
}

void AudioOutput::start()
{
    if (m_init && ma_device_start(&m_audioDevice) != MA_SUCCESS) {
        ma_device_uninit(&m_audioDevice);
        return;
}

void AudioOutput::seekTo()
{

}

qint64 AudioOutput::getAudioClockMs()
{
    return static_cast<qint64>((m_playedAudioSamples.load() * 1000) / m_sampleRate);
}