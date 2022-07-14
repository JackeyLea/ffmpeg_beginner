#include "ffmpegvideo.h"

typedef struct DecodeContext{
    AVBufferRef *hw_device_ref;
}DecodeContext;

DecodeContext decode = {NULL};

static enum AVPixelFormat hw_pix_fmt;
static AVBufferRef* hw_device_ctx=NULL;

FFmpegVideo::FFmpegVideo()
{}

FFmpegVideo::~FFmpegVideo()
{}

void FFmpegVideo::setPath(QString url)
{
    _filePath=url;
}

void FFmpegVideo::ffmpeg_init_variables()
{
    avformat_network_init();
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    yuvFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
    nv12Frame = av_frame_alloc();

    initFlag=true;
}

void FFmpegVideo::ffmpeg_free_variables()
{
    if(!pkt) av_packet_free(&pkt);
    if(!yuvFrame) av_frame_free(&yuvFrame);
    if(!rgbFrame) av_frame_free(&rgbFrame);
    if(!nv12Frame) av_frame_free(&nv12Frame);
    if(!videoCodecCtx) avcodec_free_context(&videoCodecCtx);
    if(!videoCodecCtx) avcodec_close(videoCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
}

int FFmpegVideo::open_input_file()
{
    if(!initFlag){
        ffmpeg_init_variables();
        qDebug()<<"init variables done";
    }

    enum AVHWDeviceType type;
    int i;

    type = av_hwdevice_find_type_by_name("cuda");
    if (type == AV_HWDEVICE_TYPE_NONE) {
        fprintf(stderr, "Device type %s is not supported.\n", "h264_cuvid");
        fprintf(stderr, "Available device types:");
        while((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
        fprintf(stderr, "\n");
        return -1;
    }

    /* open the input file */
    if (avformat_open_input(&fmtCtx, _filePath.toLocal8Bit().data(), NULL, NULL) != 0) {
        return -1;
    }

    if (avformat_find_stream_info(fmtCtx, NULL) < 0) {
        fprintf(stderr, "Cannot find input stream information.\n");
        return -1;
    }

    /* find the video stream information */
    ret = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &videoCodec, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot find a video stream in the input file\n");
        return -1;
    }
    videoStreamIndex = ret;

    //获取支持该decoder的hw配置型
    for (i = 0;; i++) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(videoCodec, i);
        if (!config) {
            fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    videoCodec->name, av_hwdevice_get_type_name(type));
            return -1;
        }
        if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
                config->device_type == type) {
            hw_pix_fmt = config->pix_fmt;
            break;
        }
    }

    if (!(videoCodecCtx = avcodec_alloc_context3(videoCodec)))
        return AVERROR(ENOMEM);

    videoStream = fmtCtx->streams[videoStreamIndex];
    if (avcodec_parameters_to_context(videoCodecCtx, videoStream->codecpar) < 0)
        return -1;

    videoCodecCtx->get_format  = get_hw_format;

    if (hw_decoder_init(videoCodecCtx, type) < 0)
        return -1;

    if ((ret = avcodec_open2(videoCodecCtx, videoCodec, NULL)) < 0) {
        fprintf(stderr, "Failed to open codec for stream #%u\n", videoStreamIndex);
        return -1;
    }

    img_ctx = sws_getContext(videoCodecCtx->width,
                             videoCodecCtx->height,
                             AV_PIX_FMT_NV12,
                             videoCodecCtx->width,
                             videoCodecCtx->height,
                             AV_PIX_FMT_RGB32,
                             SWS_BICUBIC,NULL,NULL,NULL);

    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32,videoCodecCtx->width,videoCodecCtx->height,1);
    out_buffer = (unsigned char *)av_malloc(numBytes*sizeof(uchar));

    int res = av_image_fill_arrays(
                rgbFrame->data,rgbFrame->linesize,
                out_buffer,AV_PIX_FMT_RGB32,
                videoCodecCtx->width,videoCodecCtx->height,1);
    if(res<0){
        qDebug()<<"Fill arrays failed.";
        return -1;
    }

    openFlag=true;
    return true;
}

AVPixelFormat FFmpegVideo::get_hw_format(AVCodecContext *ctx, const AVPixelFormat *pix_fmts)
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

int FFmpegVideo::hw_decoder_init(AVCodecContext *ctx, const AVHWDeviceType type)
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

void FFmpegVideo::stopThread()
{
    stopFlag=true;
}

void FFmpegVideo::run()
{
    if(!openFlag){
        open_input_file();
    }

    while(av_read_frame(fmtCtx,pkt)>=0){
        if(stopFlag) break;
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
                        if((ret = av_hwframe_transfer_data(nv12Frame,yuvFrame,0))<0){
                            continue;
                        }
                    }

                    sws_scale(img_ctx,
                              (const uint8_t* const*)nv12Frame->data,
                              (const int*)nv12Frame->linesize,
                              0,
                              nv12Frame->height,
                              rgbFrame->data,rgbFrame->linesize);

                    QImage img(out_buffer,
                               videoCodecCtx->width,videoCodecCtx->height,
                               QImage::Format_RGB32);
                    emit sendQImage(img);

                    QThread::msleep(30);
                }
            }
            av_packet_unref(pkt);
        }
    }
    qDebug()<<"Thread stop now";
}

FFmpegWidget::FFmpegWidget(QWidget *parent) : QWidget(parent)
{
    ffmpeg = new FFmpegVideo;
    connect(ffmpeg,SIGNAL(sendQImage(QImage)),this,SLOT(receiveQImage(QImage)));
    connect(ffmpeg,&FFmpegVideo::finished,ffmpeg,&FFmpegVideo::deleteLater);
}

FFmpegWidget::~FFmpegWidget()
{
    qDebug()<<"exit player";
    if(ffmpeg->isRunning()){
        stop();
    }
}

void FFmpegWidget::play(QString url)
{
    ffmpeg->setPath(url);
    ffmpeg->start();
}

void FFmpegWidget::stop()
{
    if(ffmpeg->isRunning()){
        ffmpeg->requestInterruption();
        ffmpeg->quit();
        ffmpeg->wait(100);
    }
    ffmpeg->ffmpeg_free_variables();
}

void FFmpegWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.drawImage(0,0,img);
}

void FFmpegWidget::receiveQImage(const QImage &rImg)
{
    img = rImg.scaled(this->size());
    update();
}
