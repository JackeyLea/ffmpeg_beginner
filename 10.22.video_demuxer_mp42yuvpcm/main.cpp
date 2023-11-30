/*
 * 音视频解码：分离出音频文件和视频文件
 * 播放命令为：ffplay -ar 44100 -ac 1 -f s16le -i out.pcm
 * ffmpeg -y -i input.mp3 -acodec pcm_s16le -f s16le -ac 2 -ar 44100 16k.pcm
 */

#include <stdio.h>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
}

/*
 * 音频播放命令：ffplay -ar 44100 -ac 1 -f s16le -i out.pcm
 */

int avcodec_save_audio_file(AVFormatContext *pFormatCtx, int streamIndex, char *fileName)
{
    const AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVCodecParameters *codecpar = pFormatCtx->streams[streamIndex]->codecpar;

    //4.获取解码器（一）:音频
    //根据索引拿到对应的流
    pCodec = avcodec_find_decoder(codecpar->codec_id);
    if (!pCodec)
    {
        printf("can't decoder audio\n");
        return -1;
    }
    //申请一个解码上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
    {
        printf("can't allocate a audio decoding context\n");
        return -1;
    }

    //用流解码信息初始化编码参数
    avcodec_parameters_to_context(pCodecCtx, codecpar);

    //没有此句会出现：Could not update timestamps for skipped samples
    pCodecCtx->pkt_timebase = pFormatCtx->streams[streamIndex]->time_base;

    //5.打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("can't open codec\n");
        return -1;
    }

//	printf("--------------- File Information ----------------\n");
//	av_dump_format(pFormatCtx, 0, fileName, 0);
//	printf("-------------------------------------------------\n");

    //编码数据
    AVPacket *packet = (AVPacket*) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();

    //frame->16bit 44100 PCM统一音频采样格式与采样率
    SwrContext *swrCtx = swr_alloc();
    //重采样设置选项-----------------------------------------------------------start
    //输入的采样格式
    AVSampleFormat inSampleFmt = pCodecCtx->sample_fmt;
    //输出的采样格式
    AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;
    //输入的采样率
    int inSampleRate = pCodecCtx->sample_rate;
    //输出的采样率
    int outSampleRate = 44100;
    //输入的声道布局
    uint64_t inChannelLayout = pCodecCtx->channel_layout;
    //输出的声道布局：CHANNEL_IN_MONO为单声道，CHANNEL_IN_STEREO为双声道
    uint64_t outChannelLayout = AV_CH_LAYOUT_MONO;

    printf("inSampleFmt = %d, inSampleRate = %d, inChannelLayout = %d， name = %s\n", inSampleFmt, inSampleRate,
            (int) inChannelLayout, pCodec->name);

    swr_alloc_set_opts(swrCtx, outChannelLayout, outSampleFmt, outSampleRate,
            inChannelLayout, inSampleFmt, inSampleRate, 0, NULL);
    swr_init(swrCtx);
    //重采样设置选项-----------------------------------------------------------end

    //获取输出的声道个数
    int outChannelNb = av_get_channel_layout_nb_channels(outChannelLayout);
    printf("outChannelNb = %d\n", outChannelNb);

    //存储PCM数据
    uint8_t *outBuffer = (uint8_t*) av_malloc(2 * 44100);

    FILE *fp = fopen(fileName, "wb");

    //回到流的初始位置
    av_seek_frame(pFormatCtx, streamIndex, 0, AVSEEK_FLAG_BACKWARD);

    //6.一帧一帧读取压缩的音频数据AVPacket
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        if (packet->stream_index == streamIndex)
        {
            //解码AVPacket --> AVFrame
            int ret = avcodec_send_packet(pCodecCtx, packet);
            if (ret < 0)
            {
                printf("Decode error\n");
                break;
            }

            if (avcodec_receive_frame(pCodecCtx, frame) >= 0)
            {
                swr_convert(swrCtx, &outBuffer, 2 * 44100, (const uint8_t**) frame->data, frame->nb_samples);
                //获取sample的size
                int outBufferSize = av_samples_get_buffer_size(NULL, outChannelNb, frame->nb_samples, outSampleFmt, 1);
                //写入文件
                fwrite(outBuffer, 1, outBufferSize, fp);
            }
        }

        av_packet_unref(packet);
    }

    fclose(fp);
    av_frame_free(&frame);
    av_free(outBuffer);
    swr_free(&swrCtx);
    avcodec_close(pCodecCtx);

    return 0;
}

/*
 * 视频播放命令：ffplay -video_size 654x368 -i out.yuv
 */
int avcodec_save_video_file(AVFormatContext *pFormatCtx, int streamIndex, char *fileName)
{
    const AVCodec *pCodec;
    AVCodecContext *pCodecCtx;
    AVCodecParameters *codecpar = pFormatCtx->streams[streamIndex]->codecpar;

    //4.获取解码器（一）:视频
    //根据索引拿到对应的流
    pCodec = avcodec_find_decoder(codecpar->codec_id);
    if (!pCodec)
    {
        printf("can't decoder audio\n");
        return -1;
    }
    //申请一个解码上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx)
    {
        printf("can't allocate a audio decoding context\n");
        return -1;
    }

    //用流解码信息初始化编码参数
    avcodec_parameters_to_context(pCodecCtx, codecpar);

    //没有此句会出现：Could not update timestamps for skipped samples
//	pCodecCtx->pkt_timebase = pFormatCtx->streams[streamIndex]->time_base;

    //5.打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("can't open codec\n");
        return -1;
    }

//	printf("--------------- File Information ----------------\n");
//	av_dump_format(pFormatCtx, 0, fileName, 0);
//	printf("-------------------------------------------------\n");

    //编码数据
    AVPacket *pPacket = (AVPacket*) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pFrameYUV = av_frame_alloc();

    unsigned char *outBuffer = (unsigned char*) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, outBuffer,
            AV_PIX_FMT_YUV420P, pCodecCtx->width,
            pCodecCtx->height, 1);

    SwsContext *pImgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    printf("width = %d, height = %d, name = %s\n", pCodecCtx->width, pCodecCtx->height, pCodec->name);

    FILE *fp = fopen(fileName, "wb+");

    //回到流的初始位置
    av_seek_frame(pFormatCtx, streamIndex, 0, AVSEEK_FLAG_BACKWARD);

    //6.一帧一帧读取压缩的视频数据AVPacket
    while (av_read_frame(pFormatCtx, pPacket) >= 0)
    {
        if (pPacket->stream_index == streamIndex)
        {
            //解码AVPacket --> AVFrame
            int ret = avcodec_send_packet(pCodecCtx, pPacket);
            if (ret < 0)
            {
                printf("Decode error\n");
                break;
            }

            if (avcodec_receive_frame(pCodecCtx, pFrame) >= 0)
            {
                sws_scale(pImgConvertCtx, pFrame->data, pFrame->linesize, 0,
                        pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);

                int y_size = pCodecCtx->width * pCodecCtx->height;
                fwrite(pFrameYUV->data[0], 1, y_size, fp);    //Y
                fwrite(pFrameYUV->data[1], 1, y_size / 4, fp);  //U
                fwrite(pFrameYUV->data[2], 1, y_size / 4, fp);  //V
            }
        }

        av_packet_unref(pPacket);
    }

    fclose(fp);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);
    av_free(outBuffer);
    avcodec_close(pCodecCtx);

    return 0;
}

int main()
{
    AVFormatContext *pFormatCtx;

    int audioStreamIndex = -1;
    int videoStreamIndex = -1;
    unsigned int i = 0;

    char inputFile[128] = "C:\\Users\\hyper\\Videos\\Sample.mkv";
    char outAudioFile[128] = "out.pcm";
    char outVideoFile[128] = "out.yuv";

    //1.注册组件
    avdevice_register_all();
    //封装格式上下文
    pFormatCtx = avformat_alloc_context();

    //2.打开输入文件
    if (avformat_open_input(&pFormatCtx, inputFile, NULL, NULL) != 0)
    {
        printf("can't open input file\n");
        return -1;
    }

    //3.获取音视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("can't find stream info\n");
        return -1;
    }

    //音视频编码，找到对应的音视频流的索引位置
    //找到音频流的索引
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audioStreamIndex = i;
            break;
        }
    }

    //找到视频流的索引
    for (i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }

    printf("audioStreamIndex = %d, videoStreamIndex = %d\n", audioStreamIndex, videoStreamIndex);

    if (audioStreamIndex == -1)
    {
        printf("can't find a audio stream\n");
    }
    else
    {
        printf("try to save audio stream\n");
        avcodec_save_audio_file(pFormatCtx, audioStreamIndex, outAudioFile);
    }

    if (videoStreamIndex == -1)
    {
        printf("can't find a video stream\n");
    }
    else
    {
        printf("try to save video stream\n");
        avcodec_save_video_file(pFormatCtx, videoStreamIndex, outVideoFile);
    }

    avformat_close_input(&pFormatCtx);

    return 0;
}
