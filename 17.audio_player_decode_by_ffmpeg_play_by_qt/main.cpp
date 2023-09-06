#include <iostream>

using namespace std;

#include <QString>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QTimer>
#include <QTest>

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

int main()
{
    QString _url="C:\\Users\\hyper\\Music\\Sample.mp3";

    QAudioOutput *audioOutput;
    QIODevice *streamOut;

    QAudioFormat audioFmt;
    audioFmt.setSampleRate(44100);
    audioFmt.setChannelCount(2);
    audioFmt.setSampleSize(16);
    audioFmt.setCodec("audio/pcm");
    audioFmt.setByteOrder(QAudioFormat::LittleEndian);
    audioFmt.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if(!info.isFormatSupported(audioFmt)){
        audioFmt = info.nearestFormat(audioFmt);
    }
    audioOutput = new QAudioOutput(audioFmt);
    audioOutput->setVolume(100);

    streamOut = audioOutput->start();

    AVFormatContext *fmtCtx =avformat_alloc_context();
    AVCodecContext *codecCtx = NULL;
    AVPacket *pkt=av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int aStreamIndex = -1;

    do{
        if(avformat_open_input(&fmtCtx,_url.toLocal8Bit().data(),NULL,NULL)<0){
            qDebug("Cannot open input file.");
            break;
        }
        if(avformat_find_stream_info(fmtCtx,NULL)<0){
            qDebug("Cannot find any stream in file.");
            break;
        }

        av_dump_format(fmtCtx,0,_url.toLocal8Bit().data(),0);

        for(size_t i=0;i<fmtCtx->nb_streams;i++){
            if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
                aStreamIndex=(int)i;
                break;
            }
        }
        if(aStreamIndex==-1){
            qDebug("Cannot find audio stream.");
            break;
        }

        AVCodecParameters *aCodecPara = fmtCtx->streams[aStreamIndex]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(aCodecPara->codec_id);
        if(!codec){
            qDebug("Cannot find any codec for audio.");
            break;
        }
        codecCtx = avcodec_alloc_context3(codec);
        if(avcodec_parameters_to_context(codecCtx,aCodecPara)<0){
            qDebug("Cannot alloc codec context.");
            break;
        }
        codecCtx->pkt_timebase = fmtCtx->streams[aStreamIndex]->time_base;

        if(avcodec_open2(codecCtx,codec,NULL)<0){
            qDebug("Cannot open audio codec.");
            break;
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

        double sleep_time=0;

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
                            //qDebug("convert length is: %d.\n",len);

                            int out_size = av_samples_get_buffer_size(0,
                                                                         out_channels,
                                                                         len,
                                                                         out_sample_fmt,
                                                                         1);
                            //qDebug("buffer size is: %d.",dst_bufsize);

                            sleep_time=(out_sample_rate*16*2/8)/out_size;

                            if(audioOutput->bytesFree()<out_size){
                                QTest::qSleep(sleep_time);
                                streamOut->write((char*)audio_out_buffer,out_size);
                            }else {
                                streamOut->write((char*)audio_out_buffer,out_size);
                            }
                            //将数据写入PCM文件
                            //fwrite(audio_out_buffer,1,dst_bufsize,file);
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }
    }while(0);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avcodec_free_context(&codecCtx);
    avformat_free_context(fmtCtx);

    streamOut->close();

    return 0;
}
