#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/imgutils.h"

int main(){
    const char inFileName[] = "beat.mp4";

    AVFilterContext *bufSinkCtx;
    AVFilterContext *bufSrcCtx;
    AVFilterGraph *filterGraph;

    //打开视频文件
    AVFormatContext *fmtCtx = avformat_alloc_context();
    if(avformat_open_input(&fmtCtx,inFileName,NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return -1;
    }

    //检索多媒体流信息
    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return -1;
    }

    //寻找视频流的第一帧
    int vStreamIndex=-1;
    for(size_t i=0;i<fmtCtx->nb_streams;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            vStreamIndex=(int)i;
            break;
        }
    }
    if(vStreamIndex==-1){
        printf("Cannot find any video stream.\n");
        return -1;
    }

    AVCodecParameters *codecPara = fmtCtx->streams[vStreamIndex]->codecpar;
    //寻找视频流的解码器
    AVCodec *codec = avcodec_find_decoder(codecPara->codec_id);
    //获取codec上下文指针
    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    if(avcodec_parameters_to_context(codecCtx,codecPara)<0){
        printf("Cannot alloc ctx from para.\n");
        return -1;
    }

    if(avcodec_open2(codecCtx,codec,NULL)<0){
        printf("Cannot open codec.\n");
        return -1;
    }

    /*
     * 开始初始化滤波器
    * Filter的具体定义，只要是libavfilter中已注册的filter，
    * 就可以直接通过查询filter名字的方法获得其具体定义，所谓
    * 定义即filter的名称、功能描述、输入输出pad、相关回调函数等
    */
    AVFilter *bufSrc = avfilter_get_by_name("buffer");/* 输入buffer filter */
    AVFilter *bufSink = avfilter_get_by_name("buffersink");/* 输出buffer filter */

    //AVFilterInOut对应buffer和buffersink这两个首尾端的filter的输入输出
    AVFilterInOut *outFilter = avfilter_inout_alloc();
    AVFilterInOut *inFilter = avfilter_inout_alloc();

    enum AVPixelFormat pixel_fmts[]={AV_PIX_FMT_YUV420P,AV_PIX_FMT_NONE};

    AVBufferSinkParams *bufSinkPara;

    filterGraph = avfilter_graph_alloc();
    char args[512]="width=320:height=240:pix_fmt=yuv410p:time_base=1/24:sar=1";

    //根据指定的Filter，这里就是buffer，构造对应的初始化参数args，二者结合即可创建Filter的示例，并放入filter_graph中
    int ret = avfilter_graph_create_filter(&bufSrcCtx,
                                           bufSrc,
                                           "in",
                                           args,NULL,
                                           filterGraph);
    if(ret<0){
        printf("Cannot create buffer source.\n");
        return -1;
    }

    /* buffer video sink: to terminate the filter chain. */
    bufSinkPara = av_buffersink_params_alloc();
    bufSinkPara->pixel_fmts = pixel_fmts;
    ret = avfilter_graph_create_filter(&bufSinkCtx,
                                       bufSink,
                                       "out",
                                       NULL,
                                       bufSinkPara,
                                       filterGraph);
    av_free(bufSinkPara);
    if(ret<0){
        printf("Cannot creat buffer sink.\n");
        return -1;
    }

    /* Endpoints for the filter graph. */
    outFilter->name = av_strdup("in");
    outFilter->filter_ctx = bufSrcCtx;
    outFilter->pad_idx =0;
    outFilter->next = NULL;

    inFilter->name = av_strdup("out");
    inFilter->filter_ctx = bufSinkCtx;
    inFilter->pad_idx = 0;
    inFilter->next = NULL;

    //filter_descr是一个filter命令，例如"overlay=iw:ih"，该函数可以解析这个命令，
    //然后自动完成FilterGraph中各个Filter之间的联接
    char filterDesc[]="overlay=iw:ih";
    ret = avfilter_graph_parse_ptr(filterGraph,
                                   filterDesc,
                                   &inFilter,
                                   &outFilter,
                                   NULL);
    if(ret<0){
        printf("Cannot parse filter desc.\n");
        return -1;
    }

    //检查当前所构造的FilterGraph的完整性与可用性
    if(avfilter_graph_config(filterGraph,NULL)<0){
        printf("Check config failed.\n");
        return -1;
    }

    AVFrame *frame = av_frame_alloc();
    //用于渲染
    AVFrame *frameRGBA = av_frame_alloc();
    if(frame==NULL || frameRGBA==NULL){
        printf("Cannot alloc frame.\n");
        return -1;
    }

    // buffer中数据就是用于渲染的,且格式为RGBA
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA,
                                            codecCtx->width,
                                            codecCtx->height,
                                            1);
    uint8_t *buffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));
    av_image_fill_arrays(frameRGBA->data,frameRGBA->linesize,
                         buffer,AV_PIX_FMT_RGBA,
                         codecCtx->width,codecCtx->height,1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *swsCtx =
            sws_getContext(codecCtx->width,
                           codecCtx->height,
                codecCtx->pix_fmt,
                codecCtx->width,
                codecCtx->height,
                AV_PIX_FMT_RGBA,
                SWS_BILINEAR,NULL,NULL,NULL);
    AVPacket *pkt = av_packet_alloc();
    while(av_read_frame(fmtCtx,pkt)>=0){
        //解码该帧
        if(pkt->stream_index == vStreamIndex){
            if(avcodec_send_packet(codecCtx,pkt)>=0){
                while(avcodec_receive_frame(codecCtx,frame)>=0){
                    //获取PTS
                    av_packet_rescale_ts(pkt,codecCtx->time_base,fmtCtx->streams[vStreamIndex]->time_base);

                    //把解码后视频帧添加到filtergraph
                    if(av_buffersrc_add_frame(bufSrcCtx,frame)<0){
                        printf("Cannot add frame to bufSrcCtx.\n");
                        break;
                    }
                    //把滤波后的视频帧从filter graph取出来
                    if(av_buffersink_get_frame(bufSinkCtx,frame)<0){
                        printf("Cannot get frame from bufSinkCtx.\n");
                        return -1;
                    }

                    //格式转换
                    sws_scale(swsCtx,
                              (const uint8_t* const *)frame->data,
                              frame->linesize,
                              0,
                              codecCtx->height,
                              frameRGBA->data,frameRGBA->linesize);
                    //display result
                }
            }
        }
        av_packet_unref(pkt);
    }

    //释放内存以及关闭文件
    av_free(buffer);
    av_free(frameRGBA);
    av_free(frame);
    avcodec_close(codecCtx);
    avformat_close_input(&fmtCtx);
    avfilter_free(bufSrcCtx);
    avfilter_free(bufSinkCtx);
    avfilter_graph_free(&filterGraph);

    sws_freeContext(swsCtx);

}
