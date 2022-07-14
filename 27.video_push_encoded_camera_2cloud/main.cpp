#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
};

int main()
{
    AVFormatContext *ifmtCtx = NULL;
    AVFormatContext *ofmtCtx = NULL;

    int ret = 0;

    do{
        AVPacket *pkt=av_packet_alloc();
        AVFrame *pFrame, *pFrameYUV;
        SwsContext *pImgConvertCtx;
        AVDictionary *params = NULL;
        const AVCodec *pCodec=NULL;
        AVCodecContext *pCodecCtx=NULL;
        unsigned char *outBuffer=NULL;
        AVCodecContext *pH264CodecCtx=NULL;
        const AVCodec *pH264Codec=NULL;
        AVDictionary *options = NULL;

        unsigned int i = 0;
        int videoIndex = -1;
        int frameIndex = 0;

        ///////////////////////////////encoding/////////////////////////

        //const char *inFilename = "video=HP HD Camera";//输入URL
        const char *inFilename = "/dev/video0";
        const char *outFilename = "rtsp://192.168.1.31/test"; //输出URL
        const char *ofmtName = NULL;

        avdevice_register_all();
        avformat_network_init();

        const AVInputFormat *ifmt = av_find_input_format("v4l2");
        if (!ifmt)
        {
            printf("can't find input format.\n");
            break;
        }

        // 1. 打开输入
        // 1.1 打开输入文件，获取封装格式相关信息
        av_dict_set_int(&options, "rtbufsize", 18432000  , 0);
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        av_dict_set(&options, "vcodec", "h264", 0);
        if ((ret = avformat_open_input(&ifmtCtx, inFilename, ifmt, &options)) < 0)
        {
            printf("can't open input file: %s\n", inFilename);
            break;
        }

        // 1.2 解码一段数据，获取流相关信息
        if ((ret = avformat_find_stream_info(ifmtCtx, 0)) < 0)
        {
            printf("failed to retrieve input stream information\n");
            break;
        }

        // 1.3 获取输入ctx
        for (i=0; i<ifmtCtx->nb_streams; ++i)
        {
            if (ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoIndex = i;
                break;
            }
        }

        printf("%s:%d, videoIndex = %d\n", __FUNCTION__, __LINE__, videoIndex);

        av_dump_format(ifmtCtx, 0, inFilename, 0);

        // 1.4 查找输入解码器
        pCodec = avcodec_find_decoder(ifmtCtx->streams[videoIndex]->codecpar->codec_id);
        if (!pCodec)
        {
            printf("can't find codec\n");
            break;
        }

        pCodecCtx = avcodec_alloc_context3(pCodec);
        if (!pCodecCtx)
        {
            printf("can't alloc codec context\n");
            break;
        }

        avcodec_parameters_to_context(pCodecCtx, ifmtCtx->streams[videoIndex]->codecpar);

        //  1.5 打开输入解码器
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        {
            printf("can't open codec\n");
            break;
        }

        ///////////////////////encoding/////////////////////////
        // 2. 打开输出 output -> encode
        // 2.1 分配输出ctx
        if (strstr(outFilename, "rtmp://"))
        {
            ofmtName = "flv";
        }
        else if (strstr(outFilename, "udp://"))
        {
            ofmtName = "mpegts";
        }
        else if(strstr(outFilename, "rtsp://"))
        {
            ofmtName = "rtsp";
        }else{
            printf("Cannot get output format.\n");
            break;
        }

        avformat_alloc_output_context2(&ofmtCtx, NULL, ofmtName, outFilename);
        if (!ofmtCtx)
        {
            printf("can't create output context\n");
            break;
        }


        // 1.6 查找H264编码器
        pH264Codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!pH264Codec)
        {
            printf("can't find h264 codec.\n");
            break;
        }

        // 1.6.1 设置参数
        pH264CodecCtx = avcodec_alloc_context3(pH264Codec);
        pH264CodecCtx->codec_id = AV_CODEC_ID_H264;
        pH264CodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        pH264CodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        pH264CodecCtx->width = pCodecCtx->width;
        pH264CodecCtx->height = pCodecCtx->height;
        pH264CodecCtx->time_base.num = 1;
        pH264CodecCtx->time_base.den = 25;	//帧率（即一秒钟多少张图片）
        pH264CodecCtx->bit_rate = 400000;	//比特率（调节这个大小可以改变编码后视频的质量）
        pH264CodecCtx->gop_size = 250;
        pH264CodecCtx->qmin = 10;
        pH264CodecCtx->qmax = 51;
        //some formats want stream headers to be separate
    //	if (pH264CodecCtx->flags & AVFMT_GLOBALHEADER)
        {
            pH264CodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        // 1.7 打开H.264编码器
        av_dict_set(&params, "preset", "superfast", 0);
        av_dict_set(&params, "tune", "zerolatency", 0);	//实现实时编码
        if (avcodec_open2(pH264CodecCtx, pH264Codec, &params) < 0)
        {
            printf("can't open video encoder.\n");
            break;
        }

        // 2.2 创建输出流
        for (i=0; i<ifmtCtx->nb_streams; ++i)
        {
            AVStream *outStream = avformat_new_stream(ofmtCtx, NULL);
            if (!outStream)
            {
                printf("failed to allocate output stream\n");
                break;
            }

            avcodec_parameters_from_context(outStream->codecpar, pH264CodecCtx);
        }

        av_dump_format(ofmtCtx, 0, outFilename, 1);

        if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE))
        {
            // 2.3 创建并初始化一个AVIOContext, 用以访问URL（outFilename）指定的资源
            ret = avio_open(&ofmtCtx->pb, outFilename, AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                printf("can't open output URL: %s\n", outFilename);
                break;
            }
        }

        // 3. 数据处理
        // 3.1 写输出文件
        ret = avformat_write_header(ofmtCtx, NULL);
        if (ret < 0)
        {
            printf("Error accourred when opening output file\n");
            break;
        }

        pFrame = av_frame_alloc();
        pFrameYUV = av_frame_alloc();

        outBuffer = (unsigned char*) av_malloc(
                av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
                        pCodecCtx->height, 1));
        av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, outBuffer,
                AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

        pImgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);


        while (1)
        {
            // 3.2 从输入流读取一个packet
            ret = av_read_frame(ifmtCtx, pkt);
            if (ret < 0)
            {
                break;
            }

            if (pkt->stream_index == videoIndex)
            {
                ret = avcodec_send_packet(pCodecCtx, pkt);
                if (ret < 0)
                {
                    printf("Decode error.\n");
                    break;
                }

                if (avcodec_receive_frame(pCodecCtx, pFrame) >= 0)
                {
                    sws_scale(pImgConvertCtx,
                            (const unsigned char* const*) pFrame->data,
                            pFrame->linesize, 0,
                            pCodecCtx->height, pFrameYUV->data,
                            pFrameYUV->linesize);

                    pFrameYUV->format = pCodecCtx->pix_fmt;
                    pFrameYUV->width = pCodecCtx->width;
                    pFrameYUV->height = pCodecCtx->height;

                    ret = avcodec_send_frame(pH264CodecCtx, pFrameYUV);
                    if (ret < 0)
                    {
                        printf("failed to encode.\n");
                        break;
                    }

                    if (avcodec_receive_packet(pH264CodecCtx, pkt) >= 0)
                    {
                        // 设置输出DTS,PTS
                        pkt->pts = pkt->dts = frameIndex * (ofmtCtx->streams[0]->time_base.den) /ofmtCtx->streams[0]->time_base.num / 25;
                        frameIndex++;

                        ret = av_interleaved_write_frame(ofmtCtx, pkt);
                        if (ret < 0)
                        {
                            printf("send packet failed: %d\n", ret);
                        }
                        else
                        {
                            printf("send %5d packet successfully!\n", frameIndex);
                        }
                    }
                }
            }

            av_packet_unref(pkt);
        }
    }while(0);

    av_write_trailer(ofmtCtx);

    avformat_close_input(&ifmtCtx);

    /* close output */
    if (ofmtCtx && !(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&ofmtCtx->pb);
    }
    avformat_free_context(ofmtCtx);

    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred\n");
        return -1;
    }

    return 0;
}
