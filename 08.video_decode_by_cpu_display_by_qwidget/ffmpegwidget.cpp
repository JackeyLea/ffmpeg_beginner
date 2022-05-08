#include "ffmpegwidget.h"

FFmpegVideo::FFmpegVideo()
{
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    yuvFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();
}

FFmpegVideo::~FFmpegVideo()
{
    if(!pkt) av_packet_free(&pkt);
    if(!yuvFrame) av_frame_free(&yuvFrame);
    if(!rgbFrame) av_frame_free(&rgbFrame);
    if(!videoCodecCtx) avcodec_free_context(&videoCodecCtx);
    if(!videoCodecCtx) avcodec_close(videoCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
}

void FFmpegVideo::setUrl(QString url)
{
    _url = url;
}

int FFmpegVideo::open_input_file()
{
    if(_url.isEmpty()) return -1;

    if(avformat_open_input(&fmtCtx,_url.toLocal8Bit().data(),NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return -1;
    }

    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return -1;
    }

    int streamCnt=fmtCtx->nb_streams;
    for(int i=0;i<streamCnt;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStreamIndex = i;
            continue;
        }
    }

    if(videoStreamIndex==-1){
        printf("Cannot find video stream in file.\n");
        return -1;
    }

    AVCodecParameters *videoCodecPara = fmtCtx->streams[videoStreamIndex]->codecpar;

    if(!(videoCodec = avcodec_find_decoder(videoCodecPara->codec_id))){
        printf("Cannot find valid decode codec.\n");
        return -1;
    }

    if(!(videoCodecCtx = avcodec_alloc_context3(videoCodec))){
        printf("Cannot find valid decode codec context.\n");
        return -1;
    }

    if(avcodec_parameters_to_context(videoCodecCtx,videoCodecPara)<0){
        printf("Cannot initialize parameters.\n");
        return -1;
    }
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

    return true;
}

void FFmpegVideo::run()
{
    if(!open_input_file()){
        qDebug()<<"Please open file first.";
        return;
    }

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
                    sws_scale(img_ctx,
                              yuvFrame->data,yuvFrame->linesize,
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
        }
    }

    qDebug()<<"All video play done";
}

FFmpegWidget::FFmpegWidget(QWidget *parent) : QWidget(parent)
{
    ffmpeg = new FFmpegVideo;
    connect(ffmpeg,SIGNAL(sendQImage(QImage)),this,SLOT(receiveQImage(QImage)));
    connect(ffmpeg,&FFmpegVideo::finished,ffmpeg,&FFmpegVideo::deleteLater);
}

FFmpegWidget::~FFmpegWidget()
{
    if(ffmpeg->isRunning()){
        stop();
    }
}

void FFmpegWidget::setUrl(QString url)
{
    ffmpeg->setUrl(url);
}

void FFmpegWidget::play()
{
    stop();
    ffmpeg->start();
}

void FFmpegWidget::stop()
{
    if(ffmpeg->isRunning()){
        ffmpeg->requestInterruption();
        ffmpeg->quit();
        ffmpeg->wait(100);
    }
    img.fill(Qt::black);
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
