#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <QObject>
#include <QDebug>


typedef struct circular_buffer{
    void *buffer;     // data buffer
    void *buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void *head;       // pointer to head
    void *tail;       // pointer to tail
} circular_buffer;



class RingBuffer : public QObject
{
    Q_OBJECT
public:
    explicit RingBuffer(size_t queueCapacity, size_t frameCount, QObject *parent=0);
    ~RingBuffer();
    void initCB();
    void readHead(void *item);
    void writeTail(void *item);
    void testHead(circular_buffer *cb, void *item);
    void purge();

    bool isFull;
    size_t m_queueCapacity;
    size_t m_frameCount;
    circular_buffer *m_cb;

private:

public slots:

signals:

};

#endif // RINGBUFFER_H
