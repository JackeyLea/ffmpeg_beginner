#include <stdio.h>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
}

int main()
{
    AVFormatContext *ifmtCtx = NULL;
    AVFormatContext *ofmtCtx = NULL;
    AVPacket pkt;

    int ret = 0;

    do{
        unsigned int i = 0;
        int videoIndex = -1;
        int frameIndex = 0;
        int64_t startTime = 0;

        const char *inFilename = "/home/jackey/Videos/test.mp4"; //输入URL
        const char *outFilename = "rtsp://localhost/test";       //输出URL
        const char *ofmtName = NULL;

        AVDictionary *dict = NULL;
        av_dict_set(&dict, "rtsp_transport", "tcp", 0);
        av_dict_set(&dict, "vcodec", "h264", 0);
        av_dict_set(&dict, "f", "rtsp", 0);

        // 1. 打开输入
        // 1.1 打开输入文件，获取封装格式相关信息
        if ((ret = avformat_open_input(&ifmtCtx, inFilename, 0, &dict)) < 0)
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
        for (i = 0; i < ifmtCtx->nb_streams; ++i)
        {
            if (ifmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoIndex = i;
                break;
            }
        }

        av_dump_format(ifmtCtx, 0, inFilename, 0);

        // 2. 打开输出
        // 2.1 分配输出ctx
        ofmtName = "rtsp";//输出格式

        avformat_alloc_output_context2(&ofmtCtx, NULL, ofmtName, outFilename);
        if (!ofmtCtx)
        {
            printf("can't create output context\n");
            break;
        }

        for (i = 0; i < ifmtCtx->nb_streams; ++i)
        {
            // 2.2 基于输入流创建输出流
            AVStream *inStream = ifmtCtx->streams[i];
            AVStream *outStream = avformat_new_stream(ofmtCtx, NULL);
            if (!outStream)
            {
                printf("failed to allocate output stream\n");
                break;
            }

            // 2.3 将当前输入流中的参数拷贝到输出流中
            ret = avcodec_parameters_copy(outStream->codecpar, inStream->codecpar);
            if (ret < 0)
            {
                printf("failed to copy codec parameters\n");
                break;
            }
        }

        av_dump_format(ofmtCtx, 0, outFilename, 1);

        if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE))
        {
            // 2.4 创建并初始化一个AVIOContext, 用以访问URL（outFilename）指定的资源
            ret = avio_open(&ofmtCtx->pb, outFilename, AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                printf("can't open output URL: %s.%d\n", outFilename, ret);
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

        startTime = av_gettime();

        while (1)
        {
            AVStream *inStream, *outStream;

            // 3.2 从输出流读取一个packet
            ret = av_read_frame(ifmtCtx, &pkt);
            if (ret < 0)
            {
                break;
            }
            //Important:Delay
            if (pkt.stream_index == videoIndex)
            {
                AVRational time_base = ifmtCtx->streams[videoIndex]->time_base;
                AVRational time_base_q = {1, AV_TIME_BASE};
                int64_t pts_time = av_rescale_q(pkt.dts, time_base, time_base_q);
                int64_t now_time = av_gettime() - startTime;
                if (pts_time > now_time)
                    av_usleep(pts_time - now_time);
            }

            inStream = ifmtCtx->streams[pkt.stream_index];
            outStream = ofmtCtx->streams[pkt.stream_index];

            /* copy packet */
            // 3.3 更新packet中的pts和dts
            // 关于AVStream.time_base(容器中的time_base)的说明：
            // 输入：输入流中含有time_base，在avformat_find_stream_info()中可取到每个流中的time_base
            // 输出：avformat_write_header()会根据输出的封装格式确定每个流的time_base并写入文件中
            // AVPacket.pts和AVPacket.dts的单位是AVStream.time_base，不同的封装格式AVStream.time_base不同
            // 所以输出文件中，每个packet需要根据输出封装格式重新计算pts和dts
            av_packet_rescale_ts(&pkt, inStream->time_base, outStream->time_base);
            pkt.pos = -1;

            if (pkt.stream_index == videoIndex)
            {
                printf("send %8d video frames to output URL\n", frameIndex);
                frameIndex++;
            }

            // 3.4 将packet写入输出
            ret = av_interleaved_write_frame(ofmtCtx, &pkt);
            if (ret < 0)
            {
                printf("Error muxing packet\n");
                break;
            }
            av_packet_unref(&pkt);
        }

        // 3.5 写输出文件尾
        av_write_trailer(ofmtCtx);
    }while(0);

    avformat_close_input(&ifmtCtx);

    /* close output */
    if (ofmtCtx && !(ofmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        avio_closep(&ofmtCtx->pb);
    }
    avformat_free_context(ofmtCtx);

    if (ret < 0 && ret != AVERROR_EOF)
    {
        printf("Error occurred\n");
        return -1;
    }

    return 0;
}
