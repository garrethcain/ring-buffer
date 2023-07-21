#ifndef AUDIOTHREAD_H
#define AUDIOTHREAD_H

#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QWaitCondition>
#include <QMutex>
#include <QThread>
#include <QDateTime>
#include "math.h"
#include "sndfile.h"
#include "ringbuffer.h"
#include "portaudio.h"

enum mode {
    ST_AT_FILE,     /* Play an audioFile */
    ST_AT_SINE_S,   /* Play a solid sineWave */
    ST_AT_SINE_C,   /* Play a few second sineWave to each Channel */
    ST_AT_EXIT
};

class AudioThread : public QObject
{
    Q_OBJECT

public:
    explicit AudioThread(QString eUID, QString sUID, QObject *parent = 0);
    ~AudioThread();
    void sineWave();
    void testChannel(int chI, int delay);
    void readFile();
    bool addTrack(QString fileName);
    bool getBuffer(void *item);
    void setTest(int chI, int delay, int framerate);

    RingBuffer *ringBuf;

    int m_frame_pos;
    int m_chan_count;
    bool working;
    double m_frame_count;
    double m_frame_rate;
    QString m_eUID;
    QString m_sUID;
    QWaitCondition bufferNotFull;


private:
    int chIdx;
    int thisChIdx;
    int m_testChI;
    int m_testDelay;
    bool m_loopChecked;
    double m_aPoint;
    double m_bPoint;
    double m_prev_time;
    QMutex mutex;
    QString m_fileName;
    SNDFILE *sndFile;
    SF_INFO *sfInfo;

    enum mode playMode;


signals:
    void sliderPosition(double);
    void playingChannel(int);
    void updateTime(double);
    void done()


public slots:
    void run();
    void setPosition(double);
    void setAPosition(double);
    void setBPosition(double);
    void loopClicked(bool);
    void quit();

};

#endif // AUDIOTHREAD_H

