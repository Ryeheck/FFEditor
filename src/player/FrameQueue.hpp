#ifndef FRAMEQUEUE_HPP
#define FRAMEQUEUE_HPP

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <cstdint>
#include <functional>
#include <qmutex.h>
#include <queue>
#include <QDebug>

extern "C" {
#include <libavutil/rational.h>
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
}

template <typename T>
class FrameQueue
{
public:
    using dtsExtractor = std::function<int64_t(const T &)>;
    using sizeExtractor = std::function<int64_t(const T &)>;

    explicit FrameQueue() {}

    ~FrameQueue() 
    {  
        clear();  
        qDebug() << "FrameQueue destroy";
    }

    bool init(dtsExtractor dtsExtract, AVRational timeBase, sizeExtractor sizeExtract)
    {
        QMutexLocker locker(&m_mutex);

        m_sizeExtract = sizeExtract;
        m_dtsExtract  = dtsExtract;
        m_timeBase    = timeBase;

        return true;
    }

    bool push(const T &value)
    {
        QMutexLocker locker(&m_mutex);

        if (m_abort) {
            qDebug() << "FrameQueue: abort";
            return false;
        }

        m_queue.push(value);
        m_condNotEmpty.wakeOne();

        if (m_sizeExtract)
            m_totalBytes += m_sizeExtract(value);

        updateDurationLocked();

        return true;
    }

    bool empty()
    {
        QMutexLocker locker(&m_mutex);
        return m_queue.empty();
    }

    bool peek(T &value)
    {
        QMutexLocker locker(&m_mutex);

        if (m_queue.empty() || m_abort)  return false;

        value = m_queue.front();
        return true;
    }

    bool pop(T &value)
    {
        QMutexLocker locker(&m_mutex);

        while (m_queue.empty() && !m_abort) 
        {
            m_condNotEmpty.wait(&m_mutex);
        }

        if (m_queue.empty()) {
            // qDebug() << "FrameQueue: queue is empty";
            return false;
        }
        if (m_abort) {
            qDebug() << "FrameQueue: abort";
            return false;
        }

        value = m_queue.front();
        m_queue.pop();
        m_condNotFull.wakeOne();

        if (m_sizeExtract)
            m_totalBytes -= m_sizeExtract(value);

        updateDurationLocked();
        
        return true;
    }

    bool isFull(int64_t durationMs, int64_t totalBytes)
    {
        QMutexLocker locker(&m_mutex);

        bool durIsFull   = (durationMs > 0) && (m_duration >= durationMs);
        bool bytesIsFull = (totalBytes > 0) && (m_totalBytes >= totalBytes);

        return durIsFull || bytesIsFull;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        
        clearLocked();
    }

    void reset()
    {
        QMutexLocker locker(&m_mutex);

        clearLocked();
        m_abort = false;
        // Another flags..
    }

    int64_t getDurationMs()
    {
        QMutexLocker locker(&m_mutex);
        return m_duration;
    }

    void abort()
    {
        QMutexLocker locker(&m_mutex);

        m_abort = true;
        m_condNotEmpty.wakeAll();
        m_condNotFull.wakeAll();
    }

private:
    void clearLocked()
    {
        while (!m_queue.empty())
        {
            freeItem(m_queue.front());
            m_queue.pop();
        }
        
        m_duration   = 0;
        m_totalBytes = 0;
        m_condNotEmpty.wakeAll();
        m_condNotFull.wakeAll();
    }

    void updateDurationLocked()
    {
        // Dur = (PTSlast - PTSfirst) * timeBase * 1000

        if (m_queue.size() < 2 || !m_dtsExtract) {
            m_duration = 0;
            return;
        }

        const int64_t lastDts  = m_dtsExtract(m_queue.back());
        const int64_t firstDts = m_dtsExtract(m_queue.front());

        int64_t diffPts = lastDts - firstDts;
        if (diffPts < 0 || m_timeBase.den < 0) {
            m_duration = 0;
            return;
        }

        m_duration = static_cast<int64_t >(diffPts * av_q2d(m_timeBase) * 1000);
    }

    QMutex m_mutex;
    QWaitCondition m_condNotEmpty;
    QWaitCondition m_condNotFull;

    template<typename U>
    void freeItem(const U &) {}
    void freeItem(AVPacket *packet) {
        if (packet)
            av_packet_free(&packet);
    }

    std::queue<T> m_queue;

    dtsExtractor m_dtsExtract   = nullptr;
    sizeExtractor m_sizeExtract = nullptr;

    AVRational m_timeBase{1, 1000};
    int64_t m_totalBytes = 0;
    int64_t m_duration   = 0;
    bool m_abort         = false;
};

#endif // FRAMEQUEUE_HPP