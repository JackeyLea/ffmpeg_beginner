#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"

int flush_encoder(AVFormatContext *fmtCtx,AVCodecContext *codecCtx,int aStreamIndex){
    int ret=0;
    AVPacket *enc_pkt=av_packet_alloc();
    enc_pkt->data=NULL;
    enc_pkt->size=0;

    if (!(codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY))
            return 0;

    printf("Flushing stream #%u encoder\n",aStreamIndex);
    if((ret=avcodec_send_frame(codecCtx,0))>=0){
        while(avcodec_receive_packet(codecCtx,enc_pkt)>=0){
            printf("success encoder 1 frame.\n");
            /* mux encoded frame */
            ret = av_write_frame(fmtCtx,enc_pkt);
            if(ret<0){
                break;
            }
        }
    }

    return ret;
}

int main()
{
    AVFormatContext *fmtCtx = NULL;
    AVCodecContext *codecCtx =NULL;
    const AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVPacket *pkt = NULL;

    fmtCtx = avformat_alloc_context();
    frame = av_frame_alloc();
    pkt = av_packet_alloc();
    pkt->data = NULL;
    pkt->size = 0;

    const char *inFileName = "s16le.pcm";
    const char *outFileName = "output.mp3";

    int ret=0;

    do{
        //==========Output information============

        if(avformat_alloc_output_context2(&fmtCtx,NULL,NULL,outFileName)<0){
            printf("Cannot alloc output file context.\n");
            break;
        }
        const AVOutputFormat *outFmt = fmtCtx->oformat;

        if(avio_open(&fmtCtx->pb,outFileName,AVIO_FLAG_READ_WRITE)<0){
            printf("Cannot open output file.\n");
            break;
        }

        AVStream *outStream = avformat_new_stream(fmtCtx,NULL);
        if(!outStream){
            printf("Cannot create a new stream to output file.\n");
            break;
        }

        //设置参数
        AVCodecParameters *codecPara = fmtCtx->streams[outStream->index]->codecpar;
        codecPara->codec_type = AVMEDIA_TYPE_AUDIO;
        codecPara->codec_id = outFmt->audio_codec;
        codecPara->sample_rate=44100;
        codecPara->channel_layout = AV_CH_LAYOUT_STEREO;
        codecPara->bit_rate = 128000;
        codecPara->format = AV_SAMPLE_FMT_FLTP;

        //查找编码器
        codec = avcodec_find_encoder(outFmt->audio_codec);
        if(codec==NULL){
            printf("Cannot find audio encoder.\n");
            break;
        }

        codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx,codecPara);
        if(codecCtx==NULL){
            printf("Cannot alloc codec ctx from para.\n");
            break;
        }

        //打开编码器
        if(avcodec_open2(codecCtx,codec,NULL)<0){
            printf("Cannot open encoder.\n");
            break;
        }

        av_dump_format(fmtCtx,0,outFileName,1);

        //===========
        frame->nb_samples = codecCtx->frame_size;
        frame->format = codecCtx->sample_fmt;
        frame->channels = 2;

        // PCM重采样
        struct SwrContext *swrCtx;
        swr_alloc_set_opts2(&swrCtx,
                           &codecCtx->ch_layout,
                           codecCtx->sample_fmt,
                           codecCtx->sample_rate,
                           &frame->ch_layout,
                           AV_SAMPLE_FMT_S16,// PCM源文件的采样格式
                           44100,0,NULL);
        swr_init(swrCtx);

        /* 分配空间 */
        uint8_t **convert_data = (uint8_t**)calloc(codecCtx->channels,sizeof(*convert_data));
        av_samples_alloc(convert_data,NULL,codecCtx->channels,codecCtx->frame_size,
                         codecCtx->sample_fmt,0);

        int size = av_samples_get_buffer_size(NULL,codecCtx->channels,
                                              codecCtx->frame_size,codecCtx->sample_fmt,1);
        uint8_t *frameBuf = (uint8_t*)av_malloc(size);
        avcodec_fill_audio_frame(frame,codecCtx->channels,codecCtx->sample_fmt,
                                 (const uint8_t*)frameBuf,size,1);

        //写帧头
        ret = avformat_write_header(fmtCtx,NULL);

        FILE *inFile = fopen(inFileName,"rb");
        if(!inFile){
            printf("Cannot open input file.\n");
            break;
        }

        for(int i=0;;i++){
            //输入一帧数据的长度
            int length = frame->nb_samples*av_get_bytes_per_sample(AV_SAMPLE_FMT_S16)*frame->channels;
            //读PCM：特意注意读取的长度，否则可能出现转码之后声音变快或者变慢
            if(fread(frameBuf,1,length,inFile)<=0){
                printf("Cannot read raw data from file.\n");
                return -1;
            }else if(feof(inFile)){
                break;
            }

            swr_convert(swrCtx,convert_data,codecCtx->frame_size,
                        (const uint8_t**)frame->data,
                        frame->nb_samples);

            //输出一帧数据的长度
            length = codecCtx->frame_size * av_get_bytes_per_sample(codecCtx->sample_fmt);
            //双通道赋值（输出的AAC为双通道）
            memcpy(frame->data[0],convert_data[0],length);
            memcpy(frame->data[1],convert_data[1],length);

            frame->pts = i*100;
            if(avcodec_send_frame(codecCtx,frame)<0){
                while(avcodec_receive_packet(codecCtx,pkt)>=0){
                    pkt->stream_index = outStream->index;
                    printf("write %4d frame, size=%d, length=%d\n",i,size,length);
                    av_write_frame(fmtCtx,pkt);
                }
            }
            av_packet_unref(pkt);
        }

        // flush encoder
        if(flush_encoder(fmtCtx,codecCtx,outStream->index)<0){
            printf("Cannot flush encoder.\n");
            return -1;
        }

        // write trailer
        av_write_trailer(fmtCtx);

        fclose(inFile);
        av_free(frameBuf);
    }while(0);

    avcodec_close(codecCtx);
    av_free(frame);
    avio_close(fmtCtx->pb);
    avformat_free_context(fmtCtx);

    return ret;
}
