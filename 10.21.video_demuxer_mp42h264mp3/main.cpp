#include <stdio.h>

//#define __STDC_CONSTANT_MACROS

extern "C"
{
#include <libavcodec/avcodec.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/mem.h>

#include <libswscale/swscale.h>

#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libpostproc/postprocess.h"
}

int open_codec_context(int *streamIndex, AVFormatContext *&ofmtCtx, AVFormatContext *ifmtCtx, AVMediaType type)
{
    AVStream *outStream = NULL, *inStream = NULL;
    int ret = -1, index = -1;

    index = av_find_best_stream(ifmtCtx, type, -1, -1, NULL, 0);
    if (index < 0)
    {
        printf("can't find %s stream in input file\n", av_get_media_type_string(type));
        return ret;
    }

    inStream = ifmtCtx->streams[index];

    outStream = avformat_new_stream(ofmtCtx, NULL);
    if (!outStream)
    {
        printf("failed to allocate output stream\n");
        return ret;
    }

    ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
    if (ret < 0)
    {
        printf("failed to copy codec parametes\n");
        return ret;
    }

    outStream->codecpar->codec_tag = 0;

    //    if (ofmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
    //    {
    //        outStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    //    }

    *streamIndex = index;

    return 0;
}

int main()
{
    AVFormatContext *ifmtCtx = NULL, *ofmtCtxAudio = NULL, *ofmtCtxVideo = NULL;
    AVPacket packet;

    int videoIndex = -1, audioIndex = -1;
    int ret = 0;

    char inFilename[128] = "C:\\Users\\hyper\\Videos\\Sample.mkv";
    char outFilenameAudio[128] = "output.aac";
    char outFilenameVideo[128] = "output.h264";

    //注册设备
    avdevice_register_all();

    do
    {
        //打开输入流
        ret = avformat_open_input(&ifmtCtx, inFilename, 0, 0);
        if (ret < 0)
        {
            printf("can't open input file\n");
            break;
        }

        //获取流信息
        ret = avformat_find_stream_info(ifmtCtx, 0);
        if (ret < 0)
        {
            printf("can't retrieve input stream information\n");
            break;
        }

        //创建输出上下文:视频
        avformat_alloc_output_context2(&ofmtCtxVideo, NULL, NULL, outFilenameVideo);
        if (!ofmtCtxVideo)
        {
            printf("can't create video output context");
            break;
        }

        //创建输出上下文：音频
        avformat_alloc_output_context2(&ofmtCtxAudio, NULL, NULL, outFilenameAudio);
        if (!ofmtCtxAudio)
        {
            printf("can't create audio output context");
            break;
        }

        ret = open_codec_context(&videoIndex, ofmtCtxVideo, ifmtCtx, AVMEDIA_TYPE_VIDEO);
        if (ret < 0)
        {
            printf("can't decode video context\n");
            break;
        }

        ret = open_codec_context(&audioIndex, ofmtCtxAudio, ifmtCtx, AVMEDIA_TYPE_AUDIO);
        if (ret < 0)
        {
            printf("can't decode video context\n");
            break;
        }

        //Dump Format------------------
        printf("\n==============Input Video=============\n");
        av_dump_format(ifmtCtx, 0, inFilename, 0);
        printf("\n==============Output Video============\n");
        av_dump_format(ofmtCtxVideo, 0, outFilenameVideo, 1);
        printf("\n==============Output Audio============\n");
        av_dump_format(ofmtCtxAudio, 0, outFilenameAudio, 1);
        printf("\n======================================\n");

        //打开输出文件:视频
        if (!(ofmtCtxVideo->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&ofmtCtxVideo->pb, outFilenameVideo, AVIO_FLAG_WRITE) < 0)
            {
                printf("can't open output file: %s\n", outFilenameVideo);
                break;
            }
        }

        //打开输出文件：音频
        if (!(ofmtCtxAudio->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&ofmtCtxAudio->pb, outFilenameAudio, AVIO_FLAG_WRITE) < 0)
            {
                printf("can't open output file: %s\n", outFilenameVideo);
                break;
            }
        }

        //写文件头
        if (avformat_write_header(ofmtCtxVideo, NULL) < 0)
        {
            printf("Error occurred when opening video output file\n");
            break;
        }

        if (avformat_write_header(ofmtCtxAudio, NULL) < 0)
        {
            printf("Error occurred when opening audio output file\n");
            break;
        }

        while (1)
        {
            AVFormatContext *ofmtCtx;
            AVStream *inStream, *outStream;

            if (av_read_frame(ifmtCtx, &packet) < 0)
            {
                break;
            }

            inStream = ifmtCtx->streams[packet.stream_index];

            if (packet.stream_index == videoIndex)
            {
                outStream = ofmtCtxVideo->streams[0];
                ofmtCtx = ofmtCtxVideo;
            }
            else if (packet.stream_index == audioIndex)
            {
                outStream = ofmtCtxAudio->streams[0];
                ofmtCtx = ofmtCtxAudio;
            }
            else
            {
                continue;
            }

            //convert PTS/DTS
            packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                                          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                                          (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
            packet.pos = -1;
            packet.stream_index = 0;

            //write
            if (av_interleaved_write_frame(ofmtCtx, &packet) < 0)
            {
                printf("Error muxing packet\n");
                break;
            }

            av_packet_unref(&packet);
        }

        //write file trailer
        av_write_trailer(ofmtCtxVideo);
        av_write_trailer(ofmtCtxAudio);
    } while (0);

    avformat_close_input(&ifmtCtx);

    if (ofmtCtxVideo && !(ofmtCtxVideo->oformat->flags & AVFMT_NOFILE))
    {
        avio_close(ofmtCtxVideo->pb);
    }

    if (ofmtCtxAudio && !(ofmtCtxAudio->oformat->flags & AVFMT_NOFILE))
    {
        avio_close(ofmtCtxAudio->pb);
    }

    avformat_free_context(ofmtCtxVideo);
    avformat_free_context(ofmtCtxAudio);

    return 0;
}
