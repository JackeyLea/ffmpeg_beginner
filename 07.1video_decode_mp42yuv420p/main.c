#include <stdio.h>
#include <stdlib.h>
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"

int main() {
    FILE *fp=fopen("result.yuv","w+b");
    if(fp==NULL){
        printf("Cannot open file.\n");
        return -1;
    }

    char filePath[]       = "/home/jackey/Videos/Sample.flv";//文件地址
    int  videoStreamIndex = -1;//视频流所在流序列中的索引
    int ret=0;//默认返回值

    //需要的变量名并初始化
    AVFormatContext *fmtCtx=NULL;
    AVPacket *pkt =NULL;
    AVCodecContext *codecCtx=NULL;
    AVCodecParameters *avCodecPara=NULL;
    const AVCodec *codec=NULL;
    AVFrame *yuvFrame = av_frame_alloc();

    do{
        //=========================== 创建AVFormatContext结构体 ===============================//
        //分配一个AVFormatContext，FFMPEG所有的操作都要通过这个AVFormatContext来进行
        fmtCtx = avformat_alloc_context();
        //==================================== 打开文件 ======================================//
        if ((ret=avformat_open_input(&fmtCtx, filePath, NULL, NULL)) != 0) {
            printf("cannot open video file\n");
            break;
        }

        //=================================== 获取视频流信息 ===================================//
        if ((ret=avformat_find_stream_info(fmtCtx, NULL)) < 0) {
            printf("cannot retrive video info\n");
            break;
        }

        //循环查找视频中包含的流信息，直到找到视频类型的流
        //便将其记录下来 保存到videoStreamIndex变量中
        for (unsigned int i = 0; i < fmtCtx->nb_streams; i++) {
            if (fmtCtx->streams[ i ]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;//找到视频流就退出
            }
        }

        //如果videoStream为-1 说明没有找到视频流
        if (videoStreamIndex == -1) {
            printf("cannot find video stream\n");
            break;
        }

        //打印输入和输出信息：长度 比特率 流格式等
        av_dump_format(fmtCtx, 0, filePath, 0);

        //=================================  查找解码器 ===================================//
        avCodecPara = fmtCtx->streams[ videoStreamIndex ]->codecpar;
        codec       = avcodec_find_decoder(avCodecPara->codec_id);
        if (codec == NULL) {
            printf("cannot find decoder\n");
            break;
        }
        //根据解码器参数来创建解码器内容
        codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx, avCodecPara);
        if (codecCtx == NULL) {
            printf("Cannot alloc context.");
            break;
        }

        //================================  打开解码器 ===================================//
        if ((ret=avcodec_open2(codecCtx, codec, NULL)) < 0) { // 具体采用什么解码器ffmpeg经过封装 我们无须知道
            printf("cannot open decoder\n");
            break;
        }

        int w=codecCtx->width;//视频宽度
        int h=codecCtx->height;//视频高度

        //=========================== 分配AVPacket结构体 ===============================//
        pkt = av_packet_alloc();                      //分配一个packet
        av_new_packet(pkt, codecCtx->width * codecCtx->height); //调整packet的数据

        //===========================  读取视频信息 ===============================//
        while (av_read_frame(fmtCtx, pkt) >= 0) { //读取的是一帧视频  数据存入一个AVPacket的结构中
            if (pkt->stream_index == videoStreamIndex){
                if (avcodec_send_packet(codecCtx, pkt) == 0){
                    while (avcodec_receive_frame(codecCtx, yuvFrame) == 0){
                        fwrite(yuvFrame->data[0],1,w*h,fp);//y
                        fwrite(yuvFrame->data[1],1,w*h/4,fp);//u
                        fwrite(yuvFrame->data[2],1,w*h/4,fp);//v
                    }
                }
            }
            av_packet_unref(pkt);//重置pkt的内容
        }
    }while(0);
    //===========================释放所有指针===============================//
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    avformat_close_input(&fmtCtx);
    avformat_free_context(fmtCtx);
    av_frame_free(&yuvFrame);

    return ret;
}
