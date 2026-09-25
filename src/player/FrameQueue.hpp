#ifndef FRAMEQUEUE_HPP
#define FRAMEQUEUE_HPP

#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <memory>
#include <queue>
#include <QDebug>

extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/packet.h>
}

using AVFramePtr = std::shared_ptr<AVFrame>;

struct videoFrame {
    AVFramePtr frame;
    qint64 posMs;
};

template <typename T>
class FrameQueue
{
public:
    explicit FrameQueue(size_t maxSize = 15) : m_maxSize(maxSize), m_abort(false) {}
    ~FrameQueue() {  clear();  }

    bool push(const T &value)
    {
        QMutexLocker locker(&m_mutex);

        // If queue > maxSize
        while (m_queue.size() >= m_maxSize && !m_abort)
        {
            m_condNotFull.wait(&m_mutex);
        }
        if (m_abort) {
            qDebug() << "FrameQueue: abort";
            return false;
        }

        m_queue.push(value);
        m_condNotEmpty.wakeOne();

        return true;
    }

    bool pop(T &value)
    {
        QMutexLocker locker(&m_mutex);

        if (m_queue.empty() || m_abort) {
            // qDebug() << "FrameQueue: queue is empty";
            return false;
        }

        value = m_queue.front();
        m_queue.pop();
        m_condNotFull.wakeOne();
        
        return true;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        
        while (!m_queue.empty())
        {
            freeItem(m_queue.front());
            m_queue.pop();
        }

        m_abort = false;
        m_condNotEmpty.wakeAll();
        m_condNotFull.wakeAll();
    }

    void abort()
    {
        QMutexLocker locker(&m_mutex);

        m_abort = true;
        m_condNotEmpty.wakeAll();
        m_condNotFull.wakeAll();
    }

private:
    QMutex m_mutex;
    QWaitCondition m_condNotEmpty;
    QWaitCondition m_condNotFull;

    template<typename U>
    void freeItem(const U& ) {}
    void freeItem(AVPacket *packet) {
        if (packet)
            av_packet_free(&packet);
    }

    std::queue<T> m_queue;

    size_t m_maxSize;
    bool m_abort;
};

#endif // FRAMEQUEUE_HPP