#include "audiothread.h"

#define QUEUE_COUNT (10)
#define QUEUE_TOO_LOW (5)
#define UPDATE_EVERY (1)

AudioThread::AudioThread(QString eUID, QString sUID, QObject *parent) :
    QObject(parent),
    m_eUID(eUID),
    m_sUID(sUID)
{
    m_frame_pos     = 0;
    m_aPoint        = 0;
    m_bPoint        = 0;
    m_chan_count    = MAX_CHANNELS;
    m_prev_time     = 0;
    working         = true;
    m_frame_count   = 0;
    chIdx           = 0;
    thisChIdx       = -1;
    sndFile         = NULL;
    m_testChI       = 0;
    m_testDelay     = 2;
    m_loopChecked   = false;
}

AudioThread::~AudioThread(){
    qDebug() << "Deleting AudioThread.";
    if(sndFile)
        sf_close(sndFile);
    //delete sfInfo;
    working = false;
    delete m_lDb;
}

void AudioThread::run(){
    /* This thread should never exit; atleast until quit gets called
     * externally.
     * The file may be at the end, but we then want to loop
     * between A & B or atleast restart the file.
     */
    ringBuf = new RingBuffer(QUEUE_COUNT, FRAMES_PER_BUFFER*m_chan_count);
    while(working){
        switch(playMode){
            case ST_AT_FILE:
                readFile();
                break;
            case ST_AT_SINE_S:
                sineWave();
                break;
            case ST_AT_SINE_C:
                testChannel(m_testChI,m_testDelay);
                break;
            default:
                readFile();
        }
        while(ringBuf->isFull && working){
            /****
             * Should we be using something different.
             * This just seems a little crude.
             * Answer: Use an QEventLoop->exec() and
             * connect() getMore signal to QEventLoop->quit();
             * Need to work out the connections though...
             * The queue seems to lock.
             ****/
            mutex.lock();
            bufferNotFull.wait(&mutex);
            mutex.unlock();
        }
        QApplication::processEvents();
    }
    qDebug() << "File read.";
    emit done();
}

void AudioThread::setTest(int chI, int delay, int framerate){
    m_testChI = chI;
    m_testDelay = delay;
    if(framerate == 1)
        m_frame_rate = 48000;
    else
        m_frame_rate = 44100;
}

void AudioThread::quit(){
    qDebug() << "AudioThread::quit called.";
    playMode = ST_AT_EXIT;
    working = false;
    bufferNotFull.wakeAll();
    ringBuf->purge();
    delete ringBuf;
}

void AudioThread::testChannel(int chI, int delay){
    /**
     * Plays a solid sine wave out a
     * particular Chan.
     *
     **/
    m_frame_count = (float)(delay * m_frame_rate);
    m_bPoint = m_frame_count;

    /*
     * This little section here handles the loop points
     * for the A & B sliders.
     */
    if(m_frame_pos >= m_bPoint){
        m_frame_pos = m_aPoint;
        if(!m_loopChecked)
            emit done();
        else if(m_frame_pos > m_bPoint)
            emit done();
    }
    if(m_frame_pos  < m_aPoint){
        m_frame_pos = m_aPoint;
    }

    /* Create a single buffer */
    float sine[FRAMES_PER_BUFFER*m_chan_count];

    /* Zero out buffer; */
    for(int i=0;i<FRAMES_PER_BUFFER*m_chan_count;i++)
        sine[i] = 0;

    for(int t=0; t<FRAMES_PER_BUFFER*m_chan_count; t=t+m_chan_count ){
        for(int nch=0;nch<m_chan_count;nch++){
            if(nch == chI){ // only write to the channel we want.
                //sine[t+nch] = (float) sin( ((double)t/(double)FRAMES_PER_BUFFER) * M_PI * 2.f );
                sine[t+nch] = (float) sin( ((double)t/(double)512) * M_PI * 2.f );
            }else{
                sine[t+nch] = (float)0; // zero out the channel we don't want.
            }
        }
    }
    m_frame_pos += FRAMES_PER_BUFFER;//*m_chan_count; // Increment so we know when to break;
    if(working)
        ringBuf->writeTail(sine);
}

void AudioThread::sineWave(){
    /* Nice tidy sine tone for
     * testing.
     */
    float sine[FRAMES_PER_BUFFER*m_chan_count];
    for(int t=0; t<FRAMES_PER_BUFFER*m_chan_count; t=t+m_chan_count ){
        for(int nch=0;nch<m_chan_count;nch++){
            sine[t+nch] = (float) sin( ((double)t/(double)FRAMES_PER_BUFFER) * M_PI * 2.f );
        }
    }
    ringBuf->writeTail(sine);
}

void AudioThread::readFile(){
    /*
     * Load the actual wave file
     * into the buffer for reading
     * elsewhere.
     */
    float sine[FRAMES_PER_BUFFER*m_chan_count];
    sf_count_t framesLeft;

    /*
     * I had to put this here
     * to get this threads eventloop
     * to acknowledge any signals
     * from ouside.
     ** Namely the loopCheck state change.
     */
    QApplication::processEvents();
    /*
     * This little section here handles the loop points
     * for the A & B sliders.
     */
    if(m_frame_pos  >= m_bPoint){
        m_frame_pos = m_aPoint;
        if(!m_loopChecked)
            emit done();
    }
    if(m_frame_pos  <= m_aPoint){
        m_frame_pos = m_aPoint;
    }

    if(m_frame_pos <= m_frame_count){
        sf_seek(sndFile, m_frame_pos, SEEK_SET);
        framesLeft = sf_readf_float(sndFile, sine, FRAMES_PER_BUFFER);
        /* Frames Left is actualy framesRead;
         * so if nothing was read, we don't
         * want to write random memory.
         */
        if(framesLeft > 0){
            ringBuf->writeTail(sine);
            m_frame_pos += FRAMES_PER_BUFFER;
        }

    }else{
        /* We're done reading this file */
    }
}

bool AudioThread::addTrack(QString fileName){
    if(fileName.isEmpty()){
        qDebug() << "AudioThread::addTrack FileName is empty, playing sine wave.";
        //m_frame_rate = 44100;
        m_chan_count = m_chan_count;
        playMode = ST_AT_SINE_C;
        return true;
    }else{
        sfInfo = new SF_INFO;
        sndFile = sf_open(fileName.toLocal8Bit(), SFM_READ, sfInfo);
        if (sndFile == NULL) {
            qDebug() << "AudioThread::openTrack Error reading source file:" << fileName << sf_strerror(sndFile);
            m_fileName = "";
            playMode = ST_AT_SINE_C;
            /* Play Stereo Sine wave */
            m_chan_count = m_chan_count;
            //m_frame_rate = 44100;
            qWarning() << QString("The Audio File: %1 could not be loaded because of: %2\nThe application has been set to play a short 2 channel sine wave.").arg(fileName).arg(sf_strerror(sndFile));
            return false;
        }else{
            /* File Loaded, so lets
             * perform some sanity
             * file and format checks.
             */
            // check format
            /* int err = sf_format_check(sfInfo);
            if(err != SF_ERR_NO_ERROR){
                qDebug() << sf_error_number(err);
                return false;
            }
            if (sfInfo->format != (SF_FORMAT_WAV | SF_FORMAT_PCM_16 | SF_FORMAT_PCM_24 | SF_FORMAT_PCM_32)) {
                qDebug() << "Input should be 16/24/32 bit Wav, format is:" << sfInfo->format;
                return false;
            } */

            // Number of channels.
            if (sfInfo->channels > MAX_CHANNELS) {
                qDebug() << "Too many channels. Max Channels:" << MAX_CHANNELS;
                return false;
            }
            m_chan_count = sfInfo->channels;
            m_frame_count = sfInfo->frames;
            m_aPoint = 0;
            m_bPoint = sfInfo->frames;
            m_frame_rate = sfInfo->samplerate;
            qDebug() << "File Loaded: " << fileName << "Format:" << sfInfo->format << "Frames:" << sfInfo->frames << "Samplerate:" << sfInfo->samplerate << "Channels:" << sfInfo->channels;
            bufferNotFull.wakeAll(); //prime the buffer.
        }
    }
    return true;
}

bool AudioThread::getBuffer(void *item){
     /* Return a part of the ringBuffer.
     * this routine is just here to add
     * some abstraction between the buffer
     * and the AudioClass.
     */
    ringBuf->readHead(item);

  /***
     * wake the thread when there are less than
     * QUEUE_TOO_LOW items left.
     ***/
    if(ringBuf->m_cb->count <= QUEUE_TOO_LOW){
        /* Should be using a
         * QEventLoop->exec() here.
         */
        bufferNotFull.wakeAll();
    }

    /* Working is always true until we're exiting. */
    return working;;
}

void AudioThread::setPosition(double value){
    /* Take our file position from back up stream
     * to update the file position to the slider.
     * TO DO: sort out this math.
     */
    qDebug() << "AudioThread::setPosition:" << value << m_sUID;
    m_frame_pos = ((double)value / 100) * m_frame_count;
}

void AudioThread::setAPosition(double value){
    m_aPoint = ((double)value / 100) * m_frame_count;
    qDebug() << m_aPoint;
}

void AudioThread::setBPosition(double value){
    m_bPoint = ((double)value / 100) * m_frame_count;
    qDebug() << m_bPoint;
}

void AudioThread::loopClicked(bool state){
    m_loopChecked = state;
}

