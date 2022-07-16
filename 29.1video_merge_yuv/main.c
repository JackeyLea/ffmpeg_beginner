#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
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

int main()
{
    int ret = 0;
    avdevice_register_all();

    AVFormatContext *inFmtCtx = avformat_alloc_context();
    AVCodecContext  *inCodecCtx = NULL;
    const AVCodec         *inCodec =NULL;
    AVPacket        *inPkt =av_packet_alloc();
    AVFrame         *srcFrame =av_frame_alloc();
    AVFrame         *yuvFrame =av_frame_alloc();

    struct SwsContext *img_ctx = NULL;

    int inVideoStreamIndex = -1;

    do{
        /////////////解码器部分//////////////////////
        //打开摄像头
        const AVInputFormat *inFmt = av_find_input_format("v4l2");
        if(avformat_open_input(&inFmtCtx,"/dev/video0",inFmt,NULL)<0){
            printf("Cannot open camera.\n");
            return -1;
        }

        if(avformat_find_stream_info(inFmtCtx,NULL)<0){
            printf("Cannot find any stream in file.\n");
            return -1;
        }

        for(size_t i=0;i<inFmtCtx->nb_streams;i++){
            if(inFmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
                inVideoStreamIndex=i;
                break;
            }
        }
        if(inVideoStreamIndex==-1){
            printf("Cannot find video stream in file.\n");
            return -1;
        }

        AVCodecParameters *inVideoCodecPara = inFmtCtx->streams[inVideoStreamIndex]->codecpar;
        if(!(inCodec=avcodec_find_decoder(inVideoCodecPara->codec_id))){
            printf("Cannot find valid video decoder.\n");
            return -1;
        }
        if(!(inCodecCtx = avcodec_alloc_context3(inCodec))){
            printf("Cannot alloc valid decode codec context.\n");
            return -1;
        }
        if(avcodec_parameters_to_context(inCodecCtx,inVideoCodecPara)<0){
            printf("Cannot initialize parameters.\n");
            return -1;
        }

        if(avcodec_open2(inCodecCtx,inCodec,NULL)<0){
            printf("Cannot open codec.\n");
            return -1;
        }

        img_ctx = sws_getContext(inCodecCtx->width,
                                 inCodecCtx->height,
                                 inCodecCtx->pix_fmt,
                                 inCodecCtx->width,
                                 inCodecCtx->height,
                                 AV_PIX_FMT_YUV420P,
                                 SWS_BICUBIC,
                                 NULL,NULL,NULL);

        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                                inCodecCtx->width,
                                                inCodecCtx->height,1);
        uint8_t* out_buffer = (unsigned char*)av_malloc(numBytes*sizeof(unsigned char));

        ret = av_image_fill_arrays(yuvFrame->data,
                                   yuvFrame->linesize,
                                   out_buffer,
                                   AV_PIX_FMT_YUV420P,
                                   inCodecCtx->width,
                                   inCodecCtx->height,
                                   1);
        if(ret<0){
            printf("Fill arrays failed.\n");
            return -1;
        }

        yuvFrame->format = inCodecCtx->pix_fmt;
        yuvFrame->width = inCodecCtx->width;
        yuvFrame->height = inCodecCtx->height;
        //////////////解码器部分结束/////////////////////

        ///////////////编解码部分//////////////////////
        AVFrame *dstFrame = av_frame_alloc();
        int numBytes2 = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                                inCodecCtx->width*2,
                                                inCodecCtx->height,1);
        uint8_t* out_buffer2 = (unsigned char*)av_malloc(numBytes2*sizeof(unsigned char));

        ret = av_image_fill_arrays(dstFrame->data,
                                   dstFrame->linesize,
                                   out_buffer2,
                                   AV_PIX_FMT_YUV420P,
                                   inCodecCtx->width*2,
                                   inCodecCtx->height,
                                   1);
        if(ret<0){
            printf("Fill arrays failed.\n");
            return -1;
        }
        dstFrame->format = inCodecCtx->pix_fmt;
        dstFrame->width = inCodecCtx->width *2;
        dstFrame->height = inCodecCtx->height;

        //set dstframe bg to black
        memset(dstFrame->data[0],0,inCodecCtx->width * inCodecCtx->height*2);
        memset(dstFrame->data[1],0x80,inCodecCtx->width *inCodecCtx->height/2);
        memset(dstFrame->data[2],0x80,inCodecCtx->width * inCodecCtx->height/2);

        int count = 0;

        FILE *fp_yuv420 = fopen("test.yuv","wb+");

        while(av_read_frame(inFmtCtx,inPkt)>=0 && count<50){
            if(inPkt->stream_index == inVideoStreamIndex){
                if(avcodec_send_packet(inCodecCtx,inPkt)>=0){
                    while((ret=avcodec_receive_frame(inCodecCtx,srcFrame))>=0){
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            return -1;
                        else if (ret < 0) {
                            fprintf(stderr, "Error during decoding\n");
                            exit(1);
                        }
                        sws_scale(img_ctx,
                                  (const uint8_t* const*)srcFrame->data,
                                  srcFrame->linesize,
                                  0,inCodecCtx->height,
                                  yuvFrame->data,yuvFrame->linesize);

                        int frameheight = inCodecCtx->height;
                        int framewidth = inCodecCtx->width;
                        printf("w -> %d h-> %d.\n",framewidth,frameheight);

                        if(yuvFrame){
                            int nYIndex =0;
                            int nUVIndex = 0;

                            for(int i=0;i<frameheight;i++){
                                //y
                                memcpy(dstFrame->data[0] + i*framewidth *2,yuvFrame->data[0]+nYIndex*framewidth,framewidth);
                                memcpy(dstFrame->data[0] + framewidth + i * framewidth *2,yuvFrame->data[0]+nYIndex*framewidth,framewidth);
                                nYIndex++;
                            }

                            for(int i=0;i<frameheight/4;i++){
                                //u
                                memcpy(dstFrame->data[1]+i*framewidth*2,yuvFrame->data[1]+nUVIndex*framewidth,framewidth);
                                memcpy(dstFrame->data[1]+framewidth+i*framewidth*2,yuvFrame->data[1]+nUVIndex*framewidth,framewidth);

                                //v
                                memcpy(dstFrame->data[2]+i*framewidth*2,yuvFrame->data[2]+nUVIndex*framewidth,framewidth);
                                memcpy(dstFrame->data[2]+framewidth+i*framewidth*2,yuvFrame->data[2]+nUVIndex*framewidth,framewidth);

                                nUVIndex++;
                            }
                        }

                        fwrite(dstFrame->data[0], 1, framewidth*frameheight * 2, fp_yuv420);
                        fwrite(dstFrame->data[1], 1, framewidth*frameheight / 2, fp_yuv420);
                        fwrite(dstFrame->data[2], 1, framewidth*frameheight / 2, fp_yuv420);

                        usleep(1000*24);
                        fflush(stdout);
                        count++;
                    }
                }
                av_packet_unref(inPkt);
            }
        }
        fclose(fp_yuv420);
        sws_freeContext(img_ctx);
        ////////////////编解码部分结束////////////////
    }while(0);

    ///////////内存释放部分/////////////////////////
    av_packet_free(&inPkt);
    avcodec_free_context(&inCodecCtx);
    avcodec_close(inCodecCtx);
    avformat_close_input(&inFmtCtx);
    av_frame_free(&srcFrame);
    av_frame_free(&yuvFrame);

    return 0;
}
