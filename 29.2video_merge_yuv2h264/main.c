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
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
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

    //打开输出文件，并填充fmtCtx数据
    AVFormatContext *outFmtCtx = avformat_alloc_context();
    const AVOutputFormat *outFmt = NULL;
    AVCodecContext *outCodecCtx=NULL;
    const AVCodec        *outCodec = NULL;
    AVStream *outVStream     = NULL;

    AVPacket *outPkt = av_packet_alloc();

    struct SwsContext *img_ctx = NULL;

    int inVideoStreamIndex = -1;

    do{
        /////////////解码器部分//////////////////////
        //打开摄像头
        const AVInputFormat *inFmt = av_find_input_format("v4l2");
        if (!inFmt)
        {
            printf("can't find input format.\n");
            break;
        }
        AVDictionary *options=NULL;
        av_dict_set_int(&options, "rtbufsize", 18432000  , 0);
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

        av_dump_format(inFmtCtx, 0, "/dev/video0", 0);

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
        //////////////解码器部分结束/////////////////////

        //////////////编码器部分开始/////////////////////
        const char *outFile = "rtsp://192.168.1.31/test";       //输出URL
        const char *ofmtName = "rtsp";//输出格式;

        if(avformat_alloc_output_context2(&outFmtCtx,NULL,ofmtName,outFile)<0){
            printf("Cannot alloc output file context.\n");
            return -1;
        }
        outFmt = outFmtCtx->oformat;

        //创建h264视频流，并设置参数
        outVStream = avformat_new_stream(outFmtCtx,outCodec);
        if(outVStream==NULL){
            printf("create new video stream fialed.\n");
            return -1;
        }
        outVStream->time_base.den=30;
        outVStream->time_base.num=1;

        //编码参数相关
        AVCodecParameters *outCodecPara = outFmtCtx->streams[outVStream->index]->codecpar;
        outCodecPara->codec_type=AVMEDIA_TYPE_VIDEO;
        outCodecPara->codec_id = outFmt->video_codec;
        outCodecPara->width = 480;
        outCodecPara->height = 360;
        outCodecPara->bit_rate = 110000;

        //查找编码器
        outCodec = avcodec_find_encoder(outFmt->video_codec);
        if(outCodec==NULL){
            printf("Cannot find any encoder.\n");
            return -1;
        }

        //设置编码器内容
        outCodecCtx = avcodec_alloc_context3(outCodec);
        avcodec_parameters_to_context(outCodecCtx,outCodecPara);
        if(outCodecCtx==NULL){
            printf("Cannot alloc output codec content.\n");
            return -1;
        }
        outCodecCtx->codec_id = outFmt->video_codec;
        outCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        outCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        outCodecCtx->width = inCodecCtx->width*2;
        outCodecCtx->height = inCodecCtx->height;
        outCodecCtx->time_base.num=1;
        outCodecCtx->time_base.den=30;
        outCodecCtx->bit_rate=110000;
        outCodecCtx->gop_size=25;

        if(outCodecCtx->codec_id==AV_CODEC_ID_H264){
            outCodecCtx->qmin=10;
            outCodecCtx->qmax=51;
            outCodecCtx->qcompress=(float)0.6;
        }else if(outCodecCtx->codec_id==AV_CODEC_ID_MPEG2VIDEO){
            outCodecCtx->max_b_frames=2;
        }else if(outCodecCtx->codec_id==AV_CODEC_ID_MPEG1VIDEO){
            outCodecCtx->mb_decision=2;
        }

        //打开编码器
        AVDictionary *dict = NULL;
        av_dict_set(&dict, "rtsp_transport", "tcp", 0);
        av_dict_set(&dict, "vcodec", "h264", 0);
        //av_dict_set(&dict, "f", "rtsp", 0);
        if(avcodec_open2(outCodecCtx,outCodec,&dict)<0){
            printf("Open encoder failed.\n");
            return -1;
        }

        av_dump_format(outFmtCtx, 0, outFile, 1);

        if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE))
        {
            // 2.3 创建并初始化一个AVIOContext, 用以访问URL（outFilename）指定的资源
            ret = avio_open(&outFmtCtx->pb, outFile, AVIO_FLAG_WRITE);
            if (ret < 0)
            {
                printf("can't open output URL: %s\n", outFile);
                break;
            }
        }
        yuvFrame->format = inCodecCtx->pix_fmt;
        yuvFrame->width = inCodecCtx->width;
        yuvFrame->height = inCodecCtx->height;
        ///////////////编码器部分结束////////////////////

        ///////////////合并部分////////////////////////
        AVFrame *dstFrame = av_frame_alloc();
        int numBytes2 = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                                                outCodecCtx->width,
                                                outCodecCtx->height,1);
        uint8_t* out_buffer2 = (unsigned char*)av_malloc(numBytes2*sizeof(unsigned char));

        ret = av_image_fill_arrays(dstFrame->data,
                                   dstFrame->linesize,
                                   out_buffer2,
                                   AV_PIX_FMT_YUV420P,
                                   outCodecCtx->width,
                                   outCodecCtx->height,
                                   1);
        if(ret<0){
            printf("Fill arrays failed.\n");
            return -1;
        }
        dstFrame->format = outCodecCtx->pix_fmt;
        dstFrame->width = outCodecCtx->width;
        dstFrame->height = outCodecCtx->height;

        //set dstframe bg to black
        memset(dstFrame->data[0],0,outCodecCtx->width * outCodecCtx->height*2);
        memset(dstFrame->data[1],0x80,outCodecCtx->width *outCodecCtx->height/2);
        memset(dstFrame->data[2],0x80,outCodecCtx->width * outCodecCtx->height/2);
        //////////////合并部分结束////////////////////////

        ///////////////编解码部分//////////////////////
        ret = avformat_write_header(outFmtCtx,NULL);

        int64_t startTime = av_gettime();

        while(av_read_frame(inFmtCtx,inPkt)>=0){
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

                        dstFrame->pts=srcFrame->pts;
                        //encode
                        if(avcodec_send_frame(outCodecCtx,dstFrame)>=0){
                            if(avcodec_receive_packet(outCodecCtx,outPkt)>=0){
                                printf("encoded one frame.\n");
                                //delay
//                                AVRational time_base = outFmtCtx->streams[inVideoStreamIndex]->time_base;
//                                   AVRational time_base_q = {1, AV_TIME_BASE};
//                                   int64_t pts_time = av_rescale_q(outPkt->dts, time_base, time_base_q);
//                                   int64_t now_time = av_gettime() - startTime;
//                                   printf("pts %ld\n",pts_time);
//                                   printf("now %ld\n",now_time);
//                                   if (pts_time > now_time)
//                                       av_usleep(pts_time - now_time);

                                av_packet_rescale_ts(outPkt,
                                                     inFmtCtx->streams[inVideoStreamIndex]->time_base,
                                                     outFmtCtx->streams[0]->time_base);
                                outPkt->pos=-1;
                                av_interleaved_write_frame(outFmtCtx,outPkt);
                                av_packet_unref(outPkt);
                            }
                        }
                        //usleep(1000*24);
                    }
                }
                av_packet_unref(inPkt);
            }
        }

        av_write_trailer(outFmtCtx);
        ////////////////编解码部分结束////////////////
    }while(0);

    ///////////内存释放部分/////////////////////////
    av_packet_free(&inPkt);
    avcodec_free_context(&inCodecCtx);
    avcodec_close(inCodecCtx);
    avformat_close_input(&inFmtCtx);
    av_frame_free(&srcFrame);
    av_frame_free(&yuvFrame);

    av_packet_free(&outPkt);
    avcodec_free_context(&outCodecCtx);
    avcodec_close(outCodecCtx);
    avformat_close_input(&outFmtCtx);

    return 0;
}
