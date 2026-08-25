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
}

using AVFramePtr = std::shared_ptr<AVFrame>;

struct videoFrame {
    AVFramePtr frame;
    qint64 posMs;
};

class frameQueue
{
public:
    explicit frameQueue(size_t maxSize = 15) : m_maxSize(maxSize), m_abort(false) {}

    bool push(const videoFrame &vFrame)
    {
        QMutexLocker locker(&m_mutex);

        // If queue > maxSize
        while (m_queue.size() >= m_maxSize && !m_abort)
        {
            m_condNotFull.wait(&m_mutex);
        }
        if (m_abort) {
            qDebug() << "frameQueue: abort";
            return false;
        }

        m_queue.push(vFrame);
        m_condNotEmpty.wakeOne();

        return true;
    }

    bool pop(videoFrame &vFrame)
    {
        QMutexLocker locker(&m_mutex);

        if (m_queue.empty()) {
            // qDebug() << "frameQueue: queue is empty";
            return false;
        }

        vFrame = m_queue.front();
        m_queue.pop();
        m_condNotFull.wakeOne();
        
        return true;
    }

    void clear()
    {
        QMutexLocker locker(&m_mutex);
        
        std::queue<videoFrame> empty;
        std::swap(m_queue, empty);

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

    std::queue<videoFrame> m_queue;

    size_t m_maxSize;
    bool m_abort;
};

#endif // FRAMEQUEUE_HPP