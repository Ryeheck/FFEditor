#include "Decoder.h"
#include <cstdint>
#include <qdebug.h>
#include <qtypes.h>
#define MINIAUDIO_IMPLEMENTATION

#include "AudioOutput.h"
#include "../EventBus.h"

#include <atomic>

extern "C" {
#include "miniaudio.h"
}

AudioOutput::AudioOutput(QObject *parent) : QObject(parent)
{
    connect(&EventBus::instance(), &EventBus::volumeChanged, this, &AudioOutput::setVolume);
    connect(&EventBus::instance(), &EventBus::durationChanged, this, &AudioOutput::setDuration);
}

AudioOutput::~AudioOutput()
{
    stop();
    qDebug() << "AudioOutput destroy";
}

bool AudioOutput::init(Decoder *decoder, uint64_t sampleRate, AudioFormat format)
{
    m_decoder = decoder;
    m_sampleRate = sampleRate;
    m_audioFormat = format == AudioFormat::Stereo ? 2 : 1;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; // Works with float32 ([-1 : 1])
    config.playback.channels = m_audioFormat; // Format audio
    config.sampleRate        = m_sampleRate;  // Ghz
    config.dataCallback      = audioCallback; // Function
    config.pUserData         = this;          // Ptr on this player

    if (ma_device_init(NULL, &config, &m_audioDevice) != MA_SUCCESS) {
        qDebug() << "AudioOutput: can't init audio";
        return false;
    }

    m_aFrameOffset = 0;
    m_playedAudioSamples.store(0);
    m_aFrame = audioFrame{};
    m_init = true;

    return true;
}

void AudioOutput::audioCallback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    AudioOutput *player = static_cast<AudioOutput *>(pDevice->pUserData);

    if (player)
        player->readSamples(static_cast<float *>(pOutput), frameCount);
}

void AudioOutput::readSamples(float *pOutput, ma_uint32 frameCount)
{
    size_t samplesNeeded = frameCount * m_audioFormat;
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
        stop();
        qDebug() << "AudioOutput: can't start audio";
    }
}

void AudioOutput::setVolume(const float &volume)
{
    if (m_init)
        ma_device_set_master_volume(&m_audioDevice, volume);
}

void AudioOutput::setDuration(const uint64_t &duration)
{
    m_duration = duration;
}

void AudioOutput::stop()
{
    if (m_init) {
        ma_device_uninit(&m_audioDevice);
        m_init = false;
    }
}

void AudioOutput::seekTo(qint64 posMs)
{
    uint64_t targetSamples;
    if (posMs < 0  || !m_sampleRate)  
        targetSamples = 0;
    else if (posMs > m_duration)
        targetSamples = m_duration;
    else                          
        targetSamples = (static_cast<uint64_t>(posMs) * m_sampleRate) / 1000;
    
    m_playedAudioSamples.store(targetSamples);
    m_aFrameOffset = 0;
    m_aFrame = audioFrame{};
}

qint64 AudioOutput::getAudioClockMs()
{
    if (m_playedAudioSamples.load() == 0)  return 0;
    return static_cast<qint64>((m_playedAudioSamples.load() * 1000) / m_sampleRate);
}