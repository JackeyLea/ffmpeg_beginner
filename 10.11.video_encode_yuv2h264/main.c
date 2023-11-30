#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"

//刷新编码器
int flush_encoder(AVFormatContext *fmtCtx, AVCodecContext *codecCtx,int vStreamIndex) {
    int      ret;
    AVPacket *enc_pkt=av_packet_alloc();
    enc_pkt->data = NULL;
    enc_pkt->size = 0;

    if (!(codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;

    printf("Flushing stream #%u encoder\n",vStreamIndex);
    if((ret=avcodec_send_frame(codecCtx,0))>=0){
        while(avcodec_receive_packet(codecCtx,enc_pkt)>=0){
            printf("success encoder 1 frame.\n");

            // parpare packet for muxing
            enc_pkt->stream_index = vStreamIndex;
            av_packet_rescale_ts(enc_pkt,codecCtx->time_base,
                                 fmtCtx->streams[ vStreamIndex ]->time_base);
            ret = av_interleaved_write_frame(fmtCtx, enc_pkt);
            if(ret<0){
                break;
            }
        }
    }

    return ret;
}

int main()
{
    AVFormatContext *fmtCtx = NULL;
    const AVOutputFormat *outFmt = NULL;
    AVStream *vStream = NULL;
    AVCodecContext *codecCtx = NULL;
    const AVCodec *codec = NULL;
    AVPacket *pkt=av_packet_alloc(); //创建已编码帧

    uint8_t *picture_buf = NULL;
    AVFrame *picFrame = NULL;
    size_t size;
    int ret = -1;

    //[1]!打开视频文件
    FILE *in_file = fopen("result.yuv", "rb");
    if (!in_file) {
        printf("can not open file!\n");
        return -1;
    }
    //[1]!

    do{
        //[2]!打开输出文件，并填充fmtCtx数据
        // TODO 根据需要修改参数
        int in_w = 1280,in_h=720,frameCnt=23005;
        const char *outFile = "result.h264";
        if(avformat_alloc_output_context2(&fmtCtx,NULL,NULL,outFile)<0){
            printf("Cannot alloc output file context.\n");
            break;
        }
        outFmt=fmtCtx->oformat;
        //[2]!

        //[3]!打开输出文件
        if(avio_open(&fmtCtx->pb,outFile,AVIO_FLAG_READ_WRITE)<0){
            printf("output file open failed.\n");
            break;
        }
        //[3]!

        //[4]!创建h264视频流，并设置参数
        vStream = avformat_new_stream(fmtCtx,codec);
        if(vStream ==NULL){
            printf("failed create new video stream.\n");
            break;
        }
        vStream->time_base.den=25;
        vStream->time_base.num=1;
        //[4]!

        //[5]!编码参数相关
        AVCodecParameters *codecPara= fmtCtx->streams[vStream->index]->codecpar;
        codecPara->codec_type=AVMEDIA_TYPE_VIDEO;
        codecPara->width=in_w;
        codecPara->height=in_h;
        //[5]!

        //[6]!查找编码器
        codec = avcodec_find_encoder(outFmt->video_codec);
        if(codec == NULL){
            printf("Cannot find any endcoder.\n");
            break;
        }
        //[6]!

        //[7]!设置编码器内容
        codecCtx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codecCtx,codecPara);
        if(codecCtx==NULL){
            printf("Cannot alloc context.\n");
            break;
        }

        codecCtx->codec_id      = outFmt->video_codec;
        codecCtx->codec_type    = AVMEDIA_TYPE_VIDEO;
        codecCtx->pix_fmt       = AV_PIX_FMT_YUV420P;
        codecCtx->width         = in_w;
        codecCtx->height        = in_h;
        codecCtx->time_base.num = 1;
        codecCtx->time_base.den = 25;
        codecCtx->bit_rate      = 400000;
        codecCtx->gop_size      = 12;

        if (codecCtx->codec_id == AV_CODEC_ID_H264) {
            codecCtx->qmin      = 10;
            codecCtx->qmax      = 51;
            codecCtx->qcompress = (float)0.6;
        }
        if (codecCtx->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            codecCtx->max_b_frames = 2;
        if (codecCtx->codec_id == AV_CODEC_ID_MPEG1VIDEO)
            codecCtx->mb_decision = 2;
        //[7]!

        //[8]!打开编码器
        if(avcodec_open2(codecCtx,codec,NULL)<0){
            printf("Open encoder failed.\n");
            break;
        }
        //[8]!

        av_dump_format(fmtCtx,0,outFile,1);//输出 输出文件流信息

        //初始化帧
        picFrame         = av_frame_alloc();
        picFrame->width  = codecCtx->width;
        picFrame->height = codecCtx->height;
        picFrame->format = codecCtx->pix_fmt;
        size            = (size_t)av_image_get_buffer_size(codecCtx->pix_fmt,codecCtx->width,codecCtx->height,1);
        picture_buf     = (uint8_t *)av_malloc(size);
        av_image_fill_arrays(picFrame->data,picFrame->linesize,
                             picture_buf,codecCtx->pix_fmt,
                             codecCtx->width,codecCtx->height,1);

        //[9] --写头文件
        ret = avformat_write_header(fmtCtx, NULL);
        //[9]

        int      y_size = codecCtx->width * codecCtx->height;
        av_new_packet(pkt, (int)(size * 3));

        //[10] --循环编码每一帧
        for (int i = 0; i < frameCnt; i++) {
            //读入YUV
            if (fread(picture_buf, 1, (unsigned long)(y_size * 3 / 2), in_file) <= 0) {
                printf("read file fail!\n");
                return -1;
            } else if (feof(in_file))
                break;

            picFrame->data[ 0 ] = picture_buf;                  //亮度Y
            picFrame->data[ 1 ] = picture_buf + y_size;         // U
            picFrame->data[ 2 ] = picture_buf + y_size * 5 / 4; // V
            // AVFrame PTS
            picFrame->pts    = i;

            //编码
            if(avcodec_send_frame(codecCtx,picFrame)>=0){
                while(avcodec_receive_packet(codecCtx,pkt)>=0){
                    printf("encoder %d success!\n",i);

                    // parpare packet for muxing
                    pkt->stream_index = vStream->index;
                    av_packet_rescale_ts(pkt, codecCtx->time_base, vStream->time_base);
                    pkt->pos = -1;
                    ret     = av_interleaved_write_frame(fmtCtx, pkt);
                    if(ret<0){
                        printf("error is: %s.\n",av_err2str(ret));
                    }

                    av_packet_unref(pkt);//刷新缓存
                }
            }
        }
        //[10]

        //[11] --Flush encoder
        ret = flush_encoder(fmtCtx,codecCtx, vStream->index);
        if (ret < 0) {
            printf("flushing encoder failed!\n");
            break;
        }
        //[11]

        //[12] --写文件尾
        av_write_trailer(fmtCtx);
        //[12]
    }while(0);

    //释放内存
    av_packet_free(&pkt);
    avcodec_close(codecCtx);
    av_free(picFrame);
    av_free(picture_buf);

    if(fmtCtx){
        avio_close(fmtCtx->pb);
        avformat_free_context(fmtCtx);
    }

    fclose(in_file);

    return 0;
}
