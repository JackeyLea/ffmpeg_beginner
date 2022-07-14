/*
 * 最简单的基于FFmpeg的音视频复用器（不涉及编码和解码）
 *
 * 本程序可以将音频码流和视频码流打包到一种封装格式中
 * 程序中将AAC编码的音频流和H.264编码的视频流打包成MP4格式文件
 *
 */

#include <stdio.h>

#define __STDC_CONSTANT_MACROS

extern "C"
{
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
}


/*
FIX: H.264 in some container format (FLV, MP4, MKV etc.) need
"h264_mp4toannexb" bitstream filter (BSF)
  *Add SPS,PPS in front of IDR frame
  *Add start code ("0,0,0,1") in front of NALU
H.264 in some container (MPEG2TS) don't need this BSF.
*/
//'1': Use H.264 Bitstream Filter
#define USE_H264BSF 0

/*
FIX:AAC in some container format (FLV, MP4, MKV etc.) need
"aac_adtstoasc" bitstream filter (BSF)
*/
//'1': Use AAC Bitstream Filter
#define USE_AACBSF 0


int main()
{
    AVFormatContext *ifmtCtxVideo = NULL, *ifmtCtxAudio = NULL, *ofmtCtx = NULL;
    AVPacket packet;

    int inVideoIndex = -1, inAudioIndex = -1;
    int outVideoIndex = -1, outAudioIndex = -1;
    int frameIndex = 0;

    int64_t curPstVideo = 0, curPstAudio = 0;

    int ret = 0;
    unsigned int i = 0;

    const char *inFilenameVideo = "input.h264";
    const char *inFilenameAudio = "input.aac";
    const char *outFilename = "output.mp4";

    //注册设备
    avdevice_register_all();

    //打开输入视频文件
    ret = avformat_open_input(&ifmtCtxVideo, inFilenameVideo, 0, 0);
    if (ret < 0)
    {
        printf("can't open input video file\n");
        goto end;
    }

    //查找输入流
    ret = avformat_find_stream_info(ifmtCtxVideo, 0);
    if (ret < 0)
    {
        printf("failed to retrieve input video stream information\n");
        goto end;
    }

    //打开输入音频文件
    ret = avformat_open_input(&ifmtCtxAudio, inFilenameAudio, 0, 0);
    if (ret < 0)
    {
        printf("can't open input audio file\n");
        goto end;
    }

    //查找输入流
    ret = avformat_find_stream_info(ifmtCtxAudio, 0);
    if (ret < 0)
    {
        printf("failed to retrieve input audio stream information\n");
        goto end;
    }

    printf("===========Input Information==========\n");
    av_dump_format(ifmtCtxVideo, 0, inFilenameVideo, 0);
    av_dump_format(ifmtCtxAudio, 0, inFilenameAudio, 0);
    printf("======================================\n");

    //新建输出上下文
    avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, outFilename);
    if (!ofmtCtx)
    {
        printf("can't create output context\n");
        goto end;
    }

    //视频输入流
    for (i = 0; i < ifmtCtxVideo->nb_streams; ++i)
    {
        if (ifmtCtxVideo->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            AVStream *inStream = ifmtCtxVideo->streams[i];
            AVStream *outStream = avformat_new_stream(ofmtCtx, NULL);
            inVideoIndex = i;

            if (!outStream)
            {
                printf("failed to allocate output stream\n");
                goto end;
            }

            outVideoIndex = outStream->index;

            if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0)
            {
                printf("faild to copy context from input to output stream");
                goto end;
            }

            outStream->codecpar->codec_tag = 0;

            break;
        }
    }

    //音频输入流
    for (i = 0; i < ifmtCtxAudio->nb_streams; ++i)
    {
        if (ifmtCtxAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            AVStream *inStream = ifmtCtxAudio->streams[i];
            AVStream *outStream = avformat_new_stream(ofmtCtx, NULL);
            inAudioIndex = i;

            if (!outStream)
            {
                printf("failed to allocate output stream\n");
                goto end;
            }

            outAudioIndex = outStream->index;

            if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0)
            {
                printf("faild to copy context from input to output stream");
                goto end;
            }

            outStream->codecpar->codec_tag = 0;

            break;
        }
    }

    printf("==========Output Information==========\n");
    av_dump_format(ofmtCtx, 0, outFilename, 1);
    printf("======================================\n");

    //打开输入文件
    if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&ofmtCtx->pb, outFilename, AVIO_FLAG_WRITE) < 0)
        {
            printf("can't open out file\n");
            goto end;
        }
    }

    //写文件头
    if (avformat_write_header(ofmtCtx, NULL) < 0)
    {
        printf("Error occurred when opening output file\n");
        goto end;
    }


#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
    AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc");
#endif
    while (1)
    {
        AVFormatContext *ifmtCtx = NULL;
        AVStream *inStream, *outStream;
        int streamIndex = 0;

        if (av_compare_ts(curPstVideo, ifmtCtxVideo->streams[inVideoIndex]->time_base, curPstAudio, ifmtCtxAudio->streams[inAudioIndex]->time_base) < 0)
        {
            ifmtCtx = ifmtCtxVideo;
            streamIndex = outVideoIndex;

            if (av_read_frame(ifmtCtx, &packet) >= 0)
            {
                do
                {
                    inStream = ifmtCtx->streams[packet.stream_index];
                    outStream = ofmtCtx->streams[streamIndex];

                    if (packet.stream_index == inVideoIndex)
                    {
                        //Fix: No PTS(Example: Raw H.264
                        //Simple Write PTS
                        if (packet.pts == AV_NOPTS_VALUE)
                        {
                            //write PTS
                            AVRational timeBase1 = inStream->time_base;
                            //Duration between 2 frames
                            int64_t calcDuration = (double)AV_TIME_BASE/av_q2d(inStream->r_frame_rate);
                            //Parameters
                            packet.pts = (double)(frameIndex*calcDuration)/(double)(av_q2d(timeBase1)*AV_TIME_BASE);
                            packet.dts = packet.pts;
                            packet.duration = (double)calcDuration/(double)(av_q2d(timeBase1)*AV_TIME_BASE);
                            frameIndex++;
                        }

                        curPstVideo = packet.pts;
                        break;
                    }
                }while(av_read_frame(ifmtCtx, &packet) >= 0);
            }
            else
            {
                break;
            }
        }
        else
        {
            ifmtCtx = ifmtCtxAudio;
            streamIndex = outAudioIndex;

            if (av_read_frame(ifmtCtx, &packet) >= 0)
            {
                do
                {
                    inStream = ifmtCtx->streams[packet.stream_index];
                    outStream = ofmtCtx->streams[streamIndex];

                    if (packet.stream_index == inAudioIndex)
                    {
                        //Fix: No PTS(Example: Raw H.264
                        //Simple Write PTS
                        if (packet.pts == AV_NOPTS_VALUE)
                        {
                            //write PTS
                            AVRational timeBase1 = inStream->time_base;
                            //Duration between 2 frames
                            int64_t calcDuration = (double)AV_TIME_BASE/av_q2d(inStream->r_frame_rate);
                            //Parameters
                            packet.pts = (double)(frameIndex*calcDuration)/(double)(av_q2d(timeBase1)*AV_TIME_BASE);
                            packet.dts = packet.pts;
                            packet.duration = (double)calcDuration/(double)(av_q2d(timeBase1)*AV_TIME_BASE);
                            frameIndex++;
                        }

                        curPstAudio = packet.pts;
                        break;
                    }
                }while(av_read_frame(ifmtCtx, &packet) >= 0);
            }
            else
            {
                break;
            }
        }

        //FIX:Bitstream Filter
#if USE_H264BSF
        av_bitstream_filter_filter(h264bsfc, inStream->codec, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);
#endif
#if USE_AACBSF
        av_bitstream_filter_filter(aacbsfc, outStream->codec, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);
#endif

        //Convert PTS/DTS
        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;
        packet.stream_index = streamIndex;

        //write
        if (av_interleaved_write_frame(ofmtCtx, &packet) < 0)
        {
            printf("error muxing packet");
            break;
        }

        av_packet_unref(&packet);
    }

    //Write file trailer
    av_write_trailer(ofmtCtx);

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
    av_bitstream_filter_close(aacbsfc);
#endif

end:
    avformat_close_input(&ifmtCtxVideo);
    avformat_close_input(&ifmtCtxAudio);
    if (ofmtCtx && !(ofmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_close(ofmtCtx->pb);

    avformat_free_context(ofmtCtx);

    return 0;
}
