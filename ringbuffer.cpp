#include "ringbuffer.h"

RingBuffer::RingBuffer(size_t queueCapacity, size_t frameCount, QObject *parent) :
    QObject(parent),
    m_queueCapacity(queueCapacity),
    m_frameCount(frameCount)
{
    initCB();
}

RingBuffer::~RingBuffer(){
    delete m_cb;
}

void RingBuffer::initCB(){
    /*
     * Initialise everything before
     * using it.
     */
    m_cb = new circular_buffer;
    m_cb->buffer = malloc(m_queueCapacity * (m_frameCount * sizeof(float)));
    if(m_cb->buffer == NULL){
        qCritical() << "RingBuffer:: circular_buffer == NULL";
        return; //TODO: CB check this.
    }

    m_cb->buffer_end    = (char *) m_cb->buffer + m_queueCapacity * (m_frameCount * sizeof(float));
    m_cb->capacity      = m_queueCapacity;
    m_cb->count         = 0;
    m_cb->sz            = m_frameCount * sizeof(float);
    m_cb->head          = m_cb->buffer;
    m_cb->tail          = m_cb->buffer;
    isFull              = false;
}

void RingBuffer::writeTail(void *item){
    if(m_cb->buffer_end < m_cb->tail){
        qCritical() << "Writing past end of buffer.";
        return; //TODO: CB check this.
    }
    /*
     * Copy buffer to the tail;
     */
    memcpy(m_cb->tail, item, m_cb->sz);
    /*
     * move the tail pointer along by sz (frame size);
     */
    m_cb->tail = (char *)m_cb->tail + m_cb->sz;
    if(m_cb->tail == m_cb->buffer_end){
        /* loop writes */
        m_cb->tail = m_cb->buffer;
    }
    m_cb->count++;
    /*
     * Has the buffer been filled yet?
     * Lets send a wait flag if it has.
     */
    if(m_cb->count >= m_cb->capacity){
        isFull = true;
    }
}

void RingBuffer::readHead(void *item){
    if(m_cb->count == 0){
        /* No Items!
         * This happens when either the io
         * has come to a hault, the buffer
         * isn't long enough, the framesPerBuffer
         * is too low or something weird has happened.
         * ie. Should never get there.
         */
        qCritical() << "RingBuffer::readHead: No Items.";
        return; //TODO: CB check this.
    }
    memcpy(item, m_cb->head, m_cb->sz);
    m_cb->head = (char*)m_cb->head + m_cb->sz;
    if(m_cb->head == m_cb->buffer_end){
        m_cb->head = m_cb->buffer;
    }
    m_cb->count--;
    if(m_cb->count < m_cb->capacity){
        isFull = false;
    }
}

void RingBuffer::purge(){
    /* Reset the count to 0
     * which means we can over right
     * each segment of the buffer.
     * Set isFull to false which
     * starts the buffer reading again.
     */
    qDebug() << "RingBuffer: purging...";
    m_cb->head = m_cb->buffer;
    m_cb->tail = m_cb->buffer;
    m_cb->count = 0;
    isFull = false;
    qDebug() << "Buffer Purged.";
}
