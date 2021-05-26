#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"

int main()
{
    const char inFileName[] = "/home/jackey/Music/test.mp3";
    const char outFileName[] = "test.pcm";
    FILE *file=fopen(outFileName,"w+b");
    if(!file){
        printf("Cannot open output file.\n");
        return -1;
    }

    AVFormatContext *fmtCtx =avformat_alloc_context();
    AVCodecContext *codecCtx = NULL;
    AVPacket *pkt=av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    int aStreamIndex = -1;

    do{

        if(avformat_open_input(&fmtCtx,inFileName,NULL,NULL)<0){
            printf("Cannot open input file.\n");
            return -1;
        }
        if(avformat_find_stream_info(fmtCtx,NULL)<0){
            printf("Cannot find any stream in file.\n");
            return -1;
        }

        av_dump_format(fmtCtx,0,inFileName,0);

        for(size_t i=0;i<fmtCtx->nb_streams;i++){
            if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
                aStreamIndex=(int)i;
                break;
            }
        }
        if(aStreamIndex==-1){
            printf("Cannot find audio stream.\n");
            return -1;
        }

        AVCodecParameters *aCodecPara = fmtCtx->streams[aStreamIndex]->codecpar;
        AVCodec *codec = avcodec_find_decoder(aCodecPara->codec_id);
        if(!codec){
            printf("Cannot find any codec for audio.\n");
            return -1;
        }
        codecCtx = avcodec_alloc_context3(codec);
        if(avcodec_parameters_to_context(codecCtx,aCodecPara)<0){
            printf("Cannot alloc codec context.\n");
            return -1;
        }
        codecCtx->pkt_timebase = fmtCtx->streams[aStreamIndex]->time_base;

        if(avcodec_open2(codecCtx,codec,NULL)<0){
            printf("Cannot open audio codec.\n");
            return -1;
        }

        while(av_read_frame(fmtCtx,pkt)>=0){
            if(pkt->stream_index==aStreamIndex){
                if(avcodec_send_packet(codecCtx,pkt)>=0){
                    while(avcodec_receive_frame(codecCtx,frame)>=0){
                        /*
                          Planar（平面），其数据格式排列方式为 (特别记住，该处是以点nb_samples采样点来交错，不是以字节交错）:
                          LLLLLLRRRRRRLLLLLLRRRRRRLLLLLLRRRRRRL...（每个LLLLLLRRRRRR为一个音频帧）
                          而不带P的数据格式（即交错排列）排列方式为：
                          LRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRLRL...（每个LR为一个音频样本）
                        */
                        if(av_sample_fmt_is_planar(codecCtx->sample_fmt)){
                            int numBytes =av_get_bytes_per_sample(codecCtx->sample_fmt);
                            //pcm播放时是LRLRLR格式，所以要交错保存数据
                            for(int i=0;i<frame->nb_samples;i++){
                                for(int ch=0;ch<codecCtx->channels;ch++){
                                    fwrite((char*)frame->data[ch]+numBytes*i,1,numBytes,file);
                                }
                            }
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

    fclose(file);

    return 0;
}
