#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"

#define MAX_AUDIO_FRAME_SIZE 192000

int main()
{
    const char inFileName[] = "C:\\Users\\hyper\\Music\\Sample.mp3";
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
            break;
        }
        if(avformat_find_stream_info(fmtCtx,NULL)<0){
            printf("Cannot find any stream in file.\n");
            break;
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
            break;
        }

        AVCodecParameters *aCodecPara = fmtCtx->streams[aStreamIndex]->codecpar;
        const AVCodec *codec = avcodec_find_decoder(aCodecPara->codec_id);
        if(!codec){
            printf("Cannot find any codec for audio.\n");
            break;
        }
        codecCtx = avcodec_alloc_context3(codec);
        if(avcodec_parameters_to_context(codecCtx,aCodecPara)<0){
            printf("Cannot alloc codec context.\n");
            break;
        }
        codecCtx->pkt_timebase=fmtCtx->streams[aStreamIndex]->time_base;

        if(avcodec_open2(codecCtx,codec,NULL)<0){
            printf("Cannot open audio codec.\n");
            break;
        }

        //设置转码参数
        uint64_t out_channel_layout = codecCtx->channel_layout;
        enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
        int out_sample_rate = codecCtx->sample_rate;
        int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);

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
                            int len = swr_convert(swr_ctx,
                                                  &audio_out_buffer,
                                                  MAX_AUDIO_FRAME_SIZE*2,
                                                  (const uint8_t**)frame->data,
                                                  frame->nb_samples);
                            if(len<=0){
                                continue;
                            }

                            int dst_bufsize = av_samples_get_buffer_size(0,
                                                                         out_channels,
                                                                         len,
                                                                         out_sample_fmt,
                                                                         1);

                            //int numBytes =av_get_bytes_per_sample(out_sample_fmt);
                            //printf("number bytes is: %d.\n",numBytes);

                            fwrite(audio_out_buffer,1,dst_bufsize,file);

                            //pcm播放时是LRLRLR格式，所以要交错保存数据
    //                        for(int i=0;i<frame->nb_samples;i++){
    //                            for(int ch=0;ch<2;ch++){
    //                                fwrite((char*)audio_out_buffer[ch]+numBytes*i,1,numBytes,file);
    //                            }
    //                        }
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
