#include <iostream>

using namespace std;

#include <QString>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QTimer>
#include <QCoreApplication>
#include <QDateTime>
#include <QTime>

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlResult>

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"
}

#define MAX_AUDIO_FRAME_SIZE 192000

void Sleep(unsigned int msec)
{
    QTime _Timer = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < _Timer ){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

//将数据读取至iobuffer的回调函数
int fill_iobuffer(void *opaque,uint8_t *buf,int buf_size){
    QByteArray *ba = static_cast<QByteArray*>(opaque);
    if(ba==NULL || ba->isEmpty()){
        return -1;
    }

    int realsize = 0;
    if(buf_size > ba->size()){
        realsize = ba->size();
        memset(buf,0,buf_size);
    }else{
        realsize=buf_size;
    }
    qDebug()<<realsize;
    memcpy(buf,ba->data(),realsize);
        qDebug()<<(int)buf[0];
        qDebug()<<ba[0].toInt();
        ba->remove(0,realsize);
        return realsize;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ///////////////get data from db////////////////////////////////
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE","test");
    db.setDatabaseName("/home/jackey/Downloads/words.sqlite3");
    if(db.open()){
        qDebug()<<QString("Connect to db %1 successfully.").arg(db.databaseName());
    }else{
        qDebug()<<QString("Cannot connect to db: %1").arg(db.lastError().text());
        return -1;
    }

    QByteArray data;
    QSqlQuery *query = new QSqlQuery(db);
    QString sql = "select word,data from a where word=\"a\";";
    if(query->exec(sql)){
        while(query->next()){
            QString word = query->value("word").toString();
            data = query->value("data").toByteArray();

            qDebug()<<QString("word: %1 data: %2").arg(word).arg(data.size());
        }
    }else{
        qDebug()<<QString("Cannot get data from db: %1").arg(query->lastError().text());
    }
    //////////////////get data from db done//////////////////////////////

    AVCodecContext *codecCtx = NULL;
    AVPacket *pkt=av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFormatContext *fmtCtx = avformat_alloc_context();
    unsigned char* iobuffer = (uchar*)av_malloc(32768);

    AVIOContext *avio = avio_alloc_context(iobuffer,32768,0,&data,fill_iobuffer,NULL,NULL);
    fmtCtx->pb=avio;

    int aStreamIndex = -1;

    do{
        if(avformat_open_input(&fmtCtx,NULL,NULL,NULL)<0){
            qDebug("Cannot open input file.");
            return -1;
        }
        if(avformat_find_stream_info(fmtCtx,NULL)<0){
            qDebug("Cannot find any stream in file.");
            return -1;
        }

        for(size_t i=0;i<fmtCtx->nb_streams;i++){
            if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
                aStreamIndex=(int)i;
                break;
            }
        }
        if(aStreamIndex==-1){
            qDebug("Cannot find audio stream.");
            return -1;
        }

        AVCodecParameters *aCodecPara = fmtCtx->streams[aStreamIndex]->codecpar;
        AVCodec *codec = avcodec_find_decoder(aCodecPara->codec_id);
        if(!codec){
            qDebug("Cannot find any codec for audio.");
            return -1;
        }
        codecCtx = avcodec_alloc_context3(codec);
        if(avcodec_parameters_to_context(codecCtx,aCodecPara)<0){
            qDebug("Cannot alloc codec context.");
            return -1;
        }
        codecCtx->pkt_timebase = fmtCtx->streams[aStreamIndex]->time_base;

        if(avcodec_open2(codecCtx,codec,NULL)<0){
            qDebug("Cannot open audio codec.");
            return -1;
        }

        //设置转码参数
        uint64_t out_channel_layout = codecCtx->channel_layout;
        enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
        int out_sample_rate = codecCtx->sample_rate;
        int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
        //printf("out rate : %d , out_channel is: %d\n",out_sample_rate,out_channels);

        uint8_t *audio_out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

        SwrContext *swr_ctx = swr_alloc_set_opts(NULL,
                                                 out_channel_layout,
                                                 out_sample_fmt,
                                                 out_sample_rate,
                                                 codecCtx->channel_layout,
                                                 codecCtx->sample_fmt,
                                                 codecCtx->sample_rate,
                                                 0,NULL);
        swr_init(swr_ctx);

        QAudioFormat audioFmt;
        audioFmt.setSampleRate(codecCtx->sample_rate);
        audioFmt.setChannelCount(out_channels);
        audioFmt.setSampleSize(16);
        audioFmt.setCodec("audio/pcm");
        audioFmt.setByteOrder(QAudioFormat::LittleEndian);
        audioFmt.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
        if(!info.isFormatSupported(audioFmt)){
            audioFmt = info.nearestFormat(audioFmt);
        }
        QAudioOutput *audioOutput = new QAudioOutput(audioFmt);
        audioOutput->setVolume(100);

        QIODevice *streamOut = audioOutput->start();

        QByteArray pcmData;

        while(av_read_frame(fmtCtx,pkt)>=0){
            if(pkt->stream_index==aStreamIndex){
                if(avcodec_send_packet(codecCtx,pkt)>=0){
                    while(avcodec_receive_frame(codecCtx,frame)>=0){
                        if(av_sample_fmt_is_planar(codecCtx->sample_fmt)){
                            int len = swr_convert(swr_ctx,
                                                  &audio_out_buffer,
                                                  MAX_AUDIO_FRAME_SIZE*2,
                                                  (const uint8_t**)frame->data,
                                                  frame->nb_samples);
                            if(len<=0){
                                continue;
                            }
                            int out_size = av_samples_get_buffer_size(0,
                                                                         out_channels,
                                                                         len,
                                                                         out_sample_fmt,
                                                                         1);
                            pcmData.append((char*)audio_out_buffer,out_size);
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }

        bool isStop = false;
        QByteArray tempBuf = pcmData;
        int chunks=0;
        while(audioOutput &&
              audioOutput->state()!=QAudio::StoppedState &&
              audioOutput->state()!=QAudio::SuspendedState &&
              !isStop){
            //play all the time if not stop
            if(tempBuf.isEmpty()){
                tempBuf = pcmData;
                Sleep(1000);
            }
            chunks = audioOutput->bytesFree()/audioOutput->periodSize();
            if(chunks){
                if(tempBuf.length() >= audioOutput->periodSize()){
                    streamOut->write(tempBuf.data(),audioOutput->periodSize());
                    tempBuf = tempBuf.mid(audioOutput->periodSize());
                }else{
                    streamOut->write(tempBuf);
                    tempBuf.clear();
                }
            }
        }
    }while(0);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avcodec_free_context(&codecCtx);
    avformat_free_context(fmtCtx);

    //streamOut->close();

    return 0;
}
