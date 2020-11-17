#include "ffmpegwidget.h"

//static QString _filePath="rtsp://192.168.1.217/stream4";
static QString _filePath = "/home/jackey/Videos/beat.mp4";

typedef struct DecodeContext{
    AVBufferRef *hw_device_ref;
}DecodeContext;

DecodeContext decode = {NULL};

FFmpegVideo::FFmpegVideo()
{}

FFmpegVideo::~FFmpegVideo()
{}

void FFmpegVideo::ffmpeg_init_variables()
{
    avformat_network_init();
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    yuvFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();

    initFlag=true;
}

void FFmpegVideo::ffmpeg_free_variables()
{
    if(!pkt) av_packet_free(&pkt);
    if(!yuvFrame) av_frame_free(&yuvFrame);
    if(!rgbFrame) av_frame_free(&rgbFrame);
    if(!videoCodecCtx) avcodec_free_context(&videoCodecCtx);
    if(!videoCodecCtx) avcodec_close(videoCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
}

int FFmpegVideo::open_input_file()
{
    if(!initFlag){
        qDebug()<<"Please init variables first.";
        return -1;
    }

    char* url=_filePath.toLatin1().data();
    if(avformat_open_input(&fmtCtx,url,NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return -1;
    }

    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return -1;
    }

    int streamCnt=fmtCtx->nb_streams;
    for(int i=0;i<streamCnt;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO &&
                fmtCtx->streams[i]->codecpar->codec_id == AV_CODEC_ID_H264){
            videoStreamIndex=i;
        }
    }

    videoStream = fmtCtx->streams[videoStreamIndex];

    if(!videoStream){
        printf("No h.264 video stream in the input file.\n");
        return  -1;
    }

    ret = av_hwdevice_ctx_create(&decode.hw_device_ref,AV_HWDEVICE_TYPE_QSV,
                                 "auto",NULL,0);
    if(ret <0){
        printf("Cannot open the hardware device.\n");
        return -1;
    }

    if(!(videoCodec = avcodec_find_decoder_by_name("h264_qsv"))){
        printf("The qsv decoder is not present in libavcodec.\n");
        return -1;
    }

    if(!(videoCodecCtx = avcodec_alloc_context3(videoCodec))){
        printf("Cannot find valid decode codec context.\n");
        return -1;
    }

    videoCodecCtx->codec_id = AV_CODEC_ID_H264;
    if(videoStream->codecpar->extradata_size){
        videoCodecCtx->extradata = (uint8_t*)av_mallocz(videoStream->codecpar->extradata_size +
                                              AV_INPUT_BUFFER_PADDING_SIZE);
        if(!videoCodecCtx->extradata){
            printf("Cannot alloc memory to data.\n");
            return -1;
        }
        memcpy(videoCodecCtx->extradata,videoStream->codecpar->extradata,
               videoStream->codecpar->extradata_size);
        videoCodecCtx->extradata_size = videoStream->codecpar->extradata_size;
    }

    videoCodecCtx->opaque = &decode;
    videoCodecCtx->pix_fmt = AV_PIX_FMT_QSV;
    //videoCodecCtx->get_format = &AV_PIX_FMT_QSV;

    if(avcodec_open2(videoCodecCtx,videoCodec,NULL)<0){
        printf("Cannot open codec.\n");
        return -1;
    }
    img_ctx = sws_getContext(videoCodecCtx->width,
                             videoCodecCtx->height,
                             videoCodecCtx->pix_fmt,
                             videoCodecCtx->width,
                             videoCodecCtx->height,
                             AV_PIX_FMT_RGB32,
                             SWS_BICUBIC,NULL,NULL,NULL);

    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32,videoCodecCtx->width,videoCodecCtx->height,1);
    out_buffer = (unsigned char *)av_malloc(numBytes*sizeof(unsigned char));

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

void FFmpegVideo::run()
{
    if(!openFlag){
        qDebug()<<"Please open file first.";
        return;
    }
    AVFrame *result = av_frame_alloc();
    while(av_read_frame(fmtCtx,pkt)>=0){
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

                    ret = av_hwframe_transfer_data(result,yuvFrame,0);
                    sws_scale(img_ctx,
                              result->data,result->linesize,
                              //yuvFrame->data,yuvFrame->linesize,
                              0,videoCodecCtx->height,
                              rgbFrame->data,rgbFrame->linesize);

                    QImage img(out_buffer,
                               videoCodecCtx->width,videoCodecCtx->height,
                               QImage::Format_RGB32);
                    emit sendQImage(img);
                    QThread::msleep(30);
                }
            }
            av_packet_unref(pkt);
            av_frame_unref(result);
        }
    }
}

FFmpegWidget::FFmpegWidget(QWidget *parent) : QWidget(parent)
{
    ffmpeg = new FFmpegVideo;
    connect(ffmpeg,SIGNAL(sendQImage(QImage)),this,SLOT(receiveQImage(QImage)));
}

FFmpegWidget::~FFmpegWidget()
{
    if(ffmpeg->isRunning()){
        ffmpeg->quit();
    }
    delete ffmpeg;
}

void FFmpegWidget::play()
{
    stop();
    ffmpeg->ffmpeg_init_variables();
    ffmpeg->open_input_file();
    ffmpeg->start();
}

void FFmpegWidget::pause()
{
}

void FFmpegWidget::stop()
{
    if(ffmpeg->isRunning()){
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
