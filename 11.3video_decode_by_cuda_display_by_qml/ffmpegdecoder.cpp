#include "ffmpegdecoder.h"

typedef struct DecodeContext{
    AVBufferRef *hw_device_ref;
}DecodeContext;

DecodeContext decode = {NULL};

static enum AVPixelFormat hw_pix_fmt;
static AVBufferRef* hw_device_ctx=NULL;

FFmpegDecoder::FFmpegDecoder()
{

}

FFmpegDecoder::~FFmpegDecoder()
{

}

void FFmpegDecoder::setUrl(QString url)
{
    _filePath = url;
}

AVPixelFormat FFmpegDecoder::get_hw_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts)
{
    Q_UNUSED(ctx)
    const enum AVPixelFormat *p;

    for (p = pix_fmts; *p != -1; p++) {
        if (*p == hw_pix_fmt)
            return *p;
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

int FFmpegDecoder::hw_decoder_init(AVCodecContext *ctx, const AVHWDeviceType type)
{
    int err = 0;

    if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
                                      NULL, NULL, 0)) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        return err;
    }
    ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

    return err;
}

void FFmpegDecoder::run()
{
    AVFormatContext *fmtCtx       =NULL;
    const AVCodec         *videoCodec   =NULL;
    AVCodecContext  *videoCodecCtx=NULL;
    AVPacket        *pkt          = NULL;
    AVFrame         *yuvFrame     = NULL;
    AVFrame         *rgbFrame     = NULL;
    AVFrame         *nv12Frame    = NULL;
    AVStream        *videoStream  = NULL;

    int videoStreamIndex =-1;
    int numBytes = -1;
    avformat_network_init();
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    yuvFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    nv12Frame = av_frame_alloc();

    enum AVHWDeviceType type;
    int i;

    type = av_hwdevice_find_type_by_name("cuda");
    if (type == AV_HWDEVICE_TYPE_NONE) {
        fprintf(stderr, "Device type %s is not supported.\n", "h264_cuvid");
        fprintf(stderr, "Available device types:");
        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
        fprintf(stderr, "\n");
        return;
    }

    /* open the input file */
    if (avformat_open_input(&fmtCtx, _filePath.toLocal8Bit().data(), NULL, NULL) != 0) {
        return;
    }

    if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return;
    }

    /* find the video stream information */
    ret = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return;
    }
    videoStreamIndex = ret;

    for (i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(videoCodec, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    videoCodec->name, av_hwdevice_get_type_name(type));
            return;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    if (!(videoCodecCtx = avcodec_alloc_context3(videoCodec)))
        return ;

    videoStream = fmtCtx->streams[videoStreamIndex];
    if (avcodec_parameters_to_context(videoCodecCtx, videoStream->codecpar) < 0)
        return;

    videoCodecCtx->get_format  = get_hw_format;

    if (hw_decoder_init(videoCodecCtx, type) < 0)
        return;

    if ((ret = avcodec_open2(videoCodecCtx, videoCodec, NULL)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", videoStreamIndex);
        return;
    }

    videoWidth = videoCodecCtx->width;
    videoHeight = videoCodecCtx->height;
    numBytes = av_image_get_buffer_size(AV_PIX_FMT_NV12,videoWidth,videoHeight,1);
    out_buffer = (uchar *)av_malloc(numBytes*sizeof(uchar));

    while(av_read_frame(fmtCtx,pkt)>=0 && !isInterruptionRequested()){
        if(pkt->stream_index == videoStreamIndex){
            if(avcodec_send_packet(videoCodecCtx,pkt)>=0){
                int ret;
                while((ret=avcodec_receive_frame(videoCodecCtx,yuvFrame))>=0){
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        return;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        exit(1);
                    }

                    if(yuvFrame->format==videoCodecCtx->pix_fmt){
                        if(av_hwframe_transfer_data(nv12Frame,yuvFrame,0)<0){
                            continue;
                        }
                    }

                    int bytes =0;
                    for(int i=0;i<videoHeight;i++){
                        memcpy(out_buffer+bytes,nv12Frame->data[0]+nv12Frame->linesize[0]*i,videoWidth);
                        bytes+=videoWidth;
                    }
                    int uv=videoHeight>>1;
                    for(int i=0;i<uv;i++){
                        memcpy(out_buffer+bytes,nv12Frame->data[1]+nv12Frame->linesize[1]*i,videoWidth);
                        bytes+=videoWidth;
                    }
                    emit sigNewFrame();

                    QThread::msleep(30);
                }
            }
            av_packet_unref(pkt);
        }
    }
    qDebug()<<"Thread stop now";

    if(!pkt) av_packet_free(&pkt);
    if(!yuvFrame) av_frame_free(&yuvFrame);
    if(!rgbFrame) av_frame_free(&rgbFrame);
    if(!nv12Frame) av_frame_free(&nv12Frame);
    if(!videoCodecCtx) avcodec_free_context(&videoCodecCtx);
    if(!videoCodecCtx) avcodec_close(videoCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
}
