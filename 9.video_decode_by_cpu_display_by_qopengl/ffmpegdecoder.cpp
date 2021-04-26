#ifndef __rasp_pi2__

#include "ffmpegdecoder.h"

FFmpegDecoder::FFmpegDecoder()
{
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    yuvFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
}

FFmpegDecoder::~FFmpegDecoder()
{
    if(!pkt) av_packet_free(&pkt);
    if(!yuvFrame) av_frame_free(&yuvFrame);
    if(!rgbFrame) av_frame_free(&rgbFrame);
    if(!videoCodecCtx) avcodec_free_context(&videoCodecCtx);
    if(!videoCodecCtx) avcodec_close(videoCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
}

void FFmpegDecoder::setUrl(const QString url)
{
    _url= url;
}

int FFmpegDecoder::width()
{
    return w;
}

int FFmpegDecoder::height()
{
    return h;
}

void FFmpegDecoder::run()
{
    if(avformat_open_input(&fmtCtx,_url.toLocal8Bit().data(),NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return;
    }

    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return;
    }

    int streamCnt=fmtCtx->nb_streams;
    for(int i=0;i<streamCnt;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStreamIndex = i;
            continue;
        }
    }

    if(videoStreamIndex==-1){
        printf("Cannot find video stream in file %s.\n",_url.toLocal8Bit().data());
        return;
    }

    AVCodecParameters *videoCodecPara = fmtCtx->streams[videoStreamIndex]->codecpar;

    if(!(videoCodec = avcodec_find_decoder(videoCodecPara->codec_id))){
        printf("Cannot find valid decode codec.\n");
        return;
    }

    if(!(videoCodecCtx = avcodec_alloc_context3(videoCodec))){
        printf("Cannot find valid decode codec context.\n");
        return;
    }

    if(avcodec_parameters_to_context(videoCodecCtx,videoCodecPara)<0){
        printf("Cannot initialize parameters.\n");
        return;
    }
    if(avcodec_open2(videoCodecCtx,videoCodec,NULL)<0){
        printf("Cannot open codec.\n");
        return;
    }

    w = videoCodecCtx->width;
    h = videoCodecCtx->height;

    numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P,w,h,1);
    out_buffer = (unsigned char *)av_malloc(numBytes*sizeof(uchar));

    while(av_read_frame(fmtCtx,pkt)>=0){
        if(pkt->stream_index == videoStreamIndex){
            if(avcodec_send_packet(videoCodecCtx,pkt)>=0){
                int ret;
                while((ret=avcodec_receive_frame(videoCodecCtx,yuvFrame))>=0){
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        return;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        continue;
                    }

                    if(isFirst){
                        isFirst=false;
                        emit sigFirst(out_buffer,w,h);
                    }

                    int bytes =0;
                    for(int i=0;i<h;i++){
                        memcpy(out_buffer+bytes,yuvFrame->data[0]+yuvFrame->linesize[0]*i,w);
                        bytes+=w;
                    }

                    int u=h>>1;
                    for(int i=0;i<u;i++){
                        memcpy(out_buffer+bytes,yuvFrame->data[1]+yuvFrame->linesize[1]*i,w/2);
                        bytes+=w/2;
                    }

                    for(int i=0;i<u;i++){
                        memcpy(out_buffer+bytes,yuvFrame->data[2]+yuvFrame->linesize[2]*i,w/2);
                        bytes+=w/2;
                    }

                    emit newFrame();

                    QThread::msleep(24);
                }
            }
            av_packet_unref(pkt);
        }
    }
}
#endif
