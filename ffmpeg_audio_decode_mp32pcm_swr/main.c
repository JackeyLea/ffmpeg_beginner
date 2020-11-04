#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"

int main()
{
    const char fileName[]="sunny.mp3";
    const char outFileName[]="sunny.pcm";
    FILE *file=fopen(outFileName,"w+b");
    if(!file){
        printf("Cannot open output file.\n");
        return -1;
    }

    AVFormatContext *fmtCtx = NULL;// ffmpeg的全局上下文，所有ffmpeg操作都需要
    AVCodecContext *codecCtx=NULL;// ffmpeg编码上下文
    AVCodec *codec=0;// ffmpeg编码器
    AVPacket *pkt=0;// ffmpeg单帧数据包
    AVFrame *frame=0;// ffmpeg单帧缓存
    struct SwrContext *swrCtx = 0;// ffmpeg音频转码

    int aStreamIndex=-1;
    int numBytes=0;
    uint8_t *outData[2]={0};
    int dstNbSamples=0;// 解码目标的采样率

    //分配
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    frame = av_frame_alloc();

    outData[0]=(uint8_t*)av_malloc(1152*8);
    outData[1]=(uint8_t*)av_malloc(1152*8);

    //打开文件(ffmpeg成功则返回0)
    if(avformat_open_input(&fmtCtx,fileName,NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return -1;
    }

    //探测流媒体信息
    if(avformat_find_stream_info(fmtCtx,0)<0){
        printf("Cannot find any stream in file.\n");
        return -1;
    }

    av_dump_format(fmtCtx,0,fileName,0);

    //提取流信息,提取视频信息
    for(size_t i=0;i<fmtCtx->nb_streams;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            aStreamIndex=(int)i;
            break;
        }
    }
    if(aStreamIndex==-1){
        printf("Cannot find audio stream in file.\n");
        return -1;
    }

    //对找到的音频流寻解码器
    AVCodecParameters *codecPara = fmtCtx->streams[aStreamIndex]->codecpar;
    codec = avcodec_find_decoder(codecPara->codec_id);
    if(!codec){
        printf("Cannot find audio decoder.\n");
        return -1;
    }

    codecCtx = avcodec_alloc_context3(codec);
    if(avcodec_parameters_to_context(codecCtx,codecPara)<0){
        printf("Cannot alloc para data to codec ctx.\n");
        return -1;
    }

    //打开解码器
    if(avcodec_open2(codecCtx,codec,NULL)<0){
        printf("Cannot open decoder for file.\n");
        return -1;
    }
    // 获取音频转码器并设置采样参数初始化
    // 通道布局与通道数据的枚举值是不同的，需要转换
    swrCtx = swr_alloc_set_opts(NULL,// 输入为空，则会分配
                                av_get_default_channel_layout(2),
                                AV_SAMPLE_FMT_S16P,// 输出的格式
                                44100,// 输出的采样频率
                                av_get_default_channel_layout(2),
                                codecCtx->sample_fmt,// 输入的格式
                                codecCtx->sample_rate,// 输入的采样率
                                0,0);
    if(swr_init(swrCtx)){
        printf("Cannot init swr ctx.\n");
        return -1;
    }

    //读取一帧数据的数据包
    while(av_read_frame(fmtCtx,pkt)>=0){
        if(pkt->stream_index==aStreamIndex){
            //将封装包发往解码器
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                //从解码器循环拿取数据帧
                while(avcodec_receive_frame(codecCtx,frame)>=0){
                    //获取每个采样点的字节大小
                    numBytes = av_get_bytes_per_sample(codecCtx->sample_fmt);
                    //修改采样率参数后，需要重新获取采样点的样本个数
                    dstNbSamples = av_rescale_rnd(frame->nb_samples,
                                                  44100,
                                                  codecCtx->sample_rate,
                                                  AV_ROUND_ZERO);
                    // 重采样
                    swr_convert(swrCtx,outData,dstNbSamples,
                                (const uint8_t**)frame->data,frame->nb_samples);
                    // 使用LRLRLRLRLRL（采样点为单位，采样点有几个字节，交替存储到文件，可使用pcm播放器播放）
                    for(int i=0;i<dstNbSamples;i++){
                        for(int ch =0;ch<codecCtx->channels;ch++){
                            fwrite((char*)outData[ch]+numBytes*i,1,numBytes,file);
                        }
                    }
                }
            }
        }
        av_packet_unref(pkt);
    }

    //释放回收资源
    av_free(outData[0]);
    av_free(outData[1]);
    outData[0]=0;
    outData[1]=0;
    swr_free(&swrCtx);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);

    printf("All done.\n");

    return 0;
}
