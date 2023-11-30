#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"

// 将H264转封装为MP4
int main(){
    int frame_index=0;//统计帧数
    int inVStreamIndex=-1,outVStreamIndex=-1;//输入输出视频流在文件中的索引位置
    const char *inVFileName = "result.h264";
    const char *outFileName = "result.mp4";

    AVFormatContext *inVFmtCtx=NULL,*outFmtCtx=NULL;
    AVCodecParameters *codecPara=NULL;
    AVStream *outVStream=NULL;
    const AVCodec *outCodec=NULL;
    AVCodecContext *outCodecCtx=NULL;
    AVCodecParameters *outCodecPara=NULL;
    AVStream *inVStream=NULL;
    AVPacket *pkt=av_packet_alloc();

    do{
        //======================输入部分============================//
        //打开输入文件
        if(avformat_open_input(&inVFmtCtx,inVFileName,NULL,NULL)<0){
            printf("Cannot open input file.\n");
            break;
        }

        //查找输入文件中的流
        if(avformat_find_stream_info(inVFmtCtx,NULL)<0){
            printf("Cannot find stream info in input file.\n");
            break;
        }

        //查找视频流在文件中的位置
        for(size_t i=0;i<inVFmtCtx->nb_streams;i++){
            if(inVFmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
                inVStreamIndex=(int)i;
                break;
            }
        }

        codecPara = inVFmtCtx->streams[inVStreamIndex]->codecpar;//输入视频流的编码参数

        printf("===============Input information========>\n");
        av_dump_format(inVFmtCtx, 0, inVFileName, 0);
        printf("===============Input information========<\n");

        //=====================输出部分=========================//
        //打开输出文件并填充格式数据
        if(avformat_alloc_output_context2(&outFmtCtx,NULL,NULL,outFileName)<0){
            printf("Cannot alloc output file context.\n");
            break;
        }

        //打开输出文件并填充数据
        if(avio_open(&outFmtCtx->pb,outFileName,AVIO_FLAG_READ_WRITE)<0){
            printf("output file open failed.\n");
            break;
        }

        //在输出的mp4文件中创建一条视频流
        outVStream = avformat_new_stream(outFmtCtx,NULL);
        if(!outVStream){
            printf("Failed allocating output stream.\n");
            break;
        }
        outVStream->time_base.den=25;
        outVStream->time_base.num=1;
        outVStreamIndex=outVStream->index;

        //查找编码器
        outCodec = avcodec_find_encoder(codecPara->codec_id);
        if(outCodec==NULL){
            printf("Cannot find any encoder.\n");
            break;
        }

        //从输入的h264编码器数据复制一份到输出文件的编码器中
        outCodecCtx=avcodec_alloc_context3(outCodec);
        outCodecPara = outFmtCtx->streams[outVStream->index]->codecpar;
        if(avcodec_parameters_copy(outCodecPara,codecPara)<0){
            printf("Cannot copy codec para.\n");
            break;
        }
        if(avcodec_parameters_to_context(outCodecCtx,outCodecPara)<0){
            printf("Cannot alloc codec ctx from para.\n");
            break;
        }
        outCodecCtx->time_base.den=25;
        outCodecCtx->time_base.num=1;

        //打开输出文件需要的编码器
        if(avcodec_open2(outCodecCtx,outCodec,NULL)<0){
            printf("Cannot open output codec.\n");
            break;
        }

        printf("============Output Information=============>\n");
        av_dump_format(outFmtCtx,0,outFileName,1);
        printf("============Output Information=============<\n");

        //写入文件头
        if(avformat_write_header(outFmtCtx,NULL)<0){
            printf("Cannot write header to file.\n");
            return -1;
        }

        //===============编码部分===============//

        inVStream = inVFmtCtx->streams[inVStreamIndex];
        while(av_read_frame(inVFmtCtx,pkt)>=0){//循环读取每一帧直到读完
            if(pkt->stream_index==inVStreamIndex){//确保处理的是视频流
                //FIXME：No PTS (Example: Raw H.264)
                //Simple Write PTS
                //如果当前处理帧的显示时间戳为0或者没有等等不是正常值
                if(pkt->pts==AV_NOPTS_VALUE){
                    printf("frame_index:%d\n", frame_index);
                    //Write PTS
                    AVRational time_base1 = inVStream->time_base;
                    //Duration between 2 frames (us)
                    int64_t calc_duration = (int)(AV_TIME_BASE / av_q2d(inVStream->r_frame_rate));
                    //Parameters
                    pkt->pts = (int)((frame_index*calc_duration) / (av_q2d(time_base1)*AV_TIME_BASE));
                    pkt->dts = pkt->pts;
                    pkt->duration = (int)(calc_duration / (av_q2d(time_base1)*AV_TIME_BASE));
                    frame_index++;
                }
                //Convert PTS/DTS
                pkt->pts = av_rescale_q_rnd(pkt->pts, inVStream->time_base, outVStream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->dts = av_rescale_q_rnd(pkt->dts, inVStream->time_base, outVStream->time_base, (enum AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                pkt->duration = av_rescale_q(pkt->duration, inVStream->time_base, outVStream->time_base);
                pkt->pos = -1;
                pkt->stream_index = outVStreamIndex;
                printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt->size, pkt->pts);
                //Write
                if (av_interleaved_write_frame(outFmtCtx, pkt) < 0) {
                    printf("Error muxing packet\n");
                    break;
                }
                av_packet_unref(pkt);
            }
        }

        av_write_trailer(outFmtCtx);
    }while(0);

    //=================释放所有指针=======================
    av_packet_free(&pkt);
    avformat_close_input(&outFmtCtx);
    avcodec_close(outCodecCtx);
    avcodec_free_context(&outCodecCtx);
    avformat_close_input(&inVFmtCtx);
    avformat_free_context(inVFmtCtx);
    avio_close(outFmtCtx->pb);

    return 0;
}
