#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"

int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int      ret;
    int      got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[ stream_index ]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);

        ret = avcodec_encode_audio2(fmt_ctx->streams[ stream_index ]->codec, &enc_pkt, NULL,
                                    &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
    AVCodecContext * pCodecCtx  = NULL;
    AVCodec *        pCodec     = NULL;
    AVFrame *        pFrame     = NULL;
    AVPacket         pkt;
    int              i = 0;
    int              ret=-1;

    const char *inFilename  = "s16le.pcm";
    const char *outFilename = "output.aac";

    avdevice_register_all();

    avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, outFilename);

    if (avio_open(&pFormatCtx->pb, outFilename, AVIO_FLAG_READ_WRITE) < 0) {
        printf("can't open output file\n");
        return -1;
    }

    AVStream *stream = avformat_new_stream(pFormatCtx, NULL);
    if (!stream) {
        printf("can't allocate new stream\n");
        return -1;
    }

    //设置参数
    pCodecCtx                 = stream->codec;
    pCodecCtx->codec_id       = pFormatCtx->oformat->audio_codec;
    pCodecCtx->codec_type     = AVMEDIA_TYPE_AUDIO;
    pCodecCtx->sample_fmt     = AV_SAMPLE_FMT_FLTP;
    pCodecCtx->sample_rate    = 44100;
    pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
    pCodecCtx->channels       = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    pCodecCtx->bit_rate       = 128000;
    //	pCodecCtx->frame_size = 1024;

    //查找编码器
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    //	pCodec = avcodec_find_encoder_by_name("libfdk_aac");
    if (!pCodec) {
        printf("can't find encoder\n");
        return -1;
    }

    //打开编码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("can't open encoder\n");
        return -1;
    }

    pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("can't alloc frame\n");
        return -1;
    }

    pFrame->nb_samples = pCodecCtx->frame_size;
    pFrame->format     = pCodecCtx->sample_fmt;
    pFrame->channels   = 2;

    // PCM重采样
    SwrContext *swrCtx = swr_alloc();

    swr_alloc_set_opts(swrCtx, av_get_default_channel_layout(pCodecCtx->channels),
                       pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
                       av_get_default_channel_layout(pFrame->channels),
                       AV_SAMPLE_FMT_S16, // PCM源文件的采样格式
                       44100, 0, NULL);
    swr_init(swrCtx);

    /* 分配空间 */
    uint8_t **convert_data = (uint8_t **)calloc(pCodecCtx->channels, sizeof(*convert_data));
    av_samples_alloc(convert_data, NULL, pCodecCtx->channels, pCodecCtx->frame_size,
                     pCodecCtx->sample_fmt, 0);

    int      size     = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size,
                                          pCodecCtx->sample_fmt, 1);
    uint8_t *frameBuf = (uint8_t *)av_malloc(size);
    avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt,
                             (const uint8_t *)frameBuf, size, 1);

    //写帧头
    ret =avformat_write_header(pFormatCtx, NULL);

    FILE *inFile = fopen(inFilename, "rb");

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    for (i = 0;; i++) {
        //输入一帧数据的长度
        int length =
            pFrame->nb_samples * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * pFrame->channels;

        //读PCM：特意注意读取的长度，否则可能出现转码之后声音变快或者变慢
        if (fread(frameBuf, 1, length, inFile) <= 0) {
            printf("failed to read raw data\n");
            return -1;
        } else if (feof(inFile)) {
            break;
        }

        swr_convert(swrCtx, convert_data, pCodecCtx->frame_size, (const uint8_t **)pFrame->data,
                    pFrame->nb_samples);

        //输出一帧数据的长度
        length = pCodecCtx->frame_size * av_get_bytes_per_sample(pCodecCtx->sample_fmt);
        //双通道赋值（输出的AAC为双通道）
        memcpy(pFrame->data[ 0 ], convert_data[ 0 ], length);
        memcpy(pFrame->data[ 1 ], convert_data[ 1 ], length);

        pFrame->pts = i * 100;

        if (avcodec_send_frame(pCodecCtx, pFrame) < 0) {
            printf("can't send frame for encoding\n");
            break;
        }

        if (avcodec_receive_packet(pCodecCtx, &pkt) >= 0) {
            pkt.stream_index = stream->index;
            printf("write %4d frame, size = %d, length = %d\n", i, size, length);
            av_write_frame(pFormatCtx, &pkt);
        }

        av_packet_unref(&pkt);
    }

    // flush encoder
    if (flush_encoder(pFormatCtx, 0) < 0) {
        printf("flushing encoder failed\n");
        return -1;
    }

    // write trailer
    av_write_trailer(pFormatCtx);

    avcodec_close(stream->codec);
    av_free(pFrame);
    av_free(frameBuf);

    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);

    fclose(inFile);

    return 0;
}
