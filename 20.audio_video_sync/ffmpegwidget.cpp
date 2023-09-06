#include "ffmpegwidget.h"

FFmpegAudio::FFmpegAudio()
{
    QAudioFormat audioFmt;
    audioFmt.setSampleRate(44100);
    audioFmt.setChannelCount(2);
    audioFmt.setSampleSize(16);
    audioFmt.setCodec("audio/pcm");
    audioFmt.setByteOrder(QAudioFormat::LittleEndian);
    audioFmt.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info = QAudioDeviceInfo::defaultOutputDevice();
    if(!info.isFormatSupported(audioFmt)){
        audioFmt = info.nearestFormat(audioFmt);
    }
    audioOutput = new QAudioOutput(audioFmt);
    audioOutput->setVolume(100);

    streamOut = audioOutput->start();

    fmtCtx = avformat_alloc_context();
    audioCodecCtx = NULL;
    pkt = av_packet_alloc();
    audioFrame = av_frame_alloc();
    swr_ctx = swr_alloc();
}

FFmpegAudio::~FFmpegAudio()
{
    if(streamOut->isOpen()){
        audioOutput->stop();
        streamOut->close();
    }
    if(!pkt) av_packet_free(&pkt);
    if(!audioCodecCtx) avcodec_free_context(&audioCodecCtx);
    if(!audioCodecCtx) avcodec_close(audioCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
}

void FFmpegAudio::setUrl(QString url)
{
    _url = url;
}

bool FFmpegAudio::open_input_file()
{
    if(_url.isEmpty()) return 0;

    if(avformat_open_input(&fmtCtx,_url.toLocal8Bit().data(),NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return 0;
    }

    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return 0;
    }

    av_dump_format(fmtCtx,0,_url.toLocal8Bit().data(),0);

    int streamCnt=fmtCtx->nb_streams;
    for(int i=0;i<streamCnt;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO){
            audioStreamIndex=(int)i;
            continue;
        }
    }

    if(audioStreamIndex==-1){
        printf("Cannot find any stream in file.\n");
        return 0;
    }

    ///////////////////////////音频部分开始//////////////////////////////////
    AVCodecParameters *audioCodecPara = fmtCtx->streams[audioStreamIndex]->codecpar;

    if(!(audioCodec = avcodec_find_decoder(audioCodecPara->codec_id))){
        printf("Cannot find valid audio decode codec.\n");
        return 0;
    }

    if(!(audioCodecCtx = avcodec_alloc_context3(audioCodec))){
        printf("Cannot find valid audio decode codec context.\n");
        return 0;
    }

    if(avcodec_parameters_to_context(audioCodecCtx,audioCodecPara)<0){
        printf("Cannot initialize audio parameters.\n");
        return 0;
    }

    audioCodecCtx->pkt_timebase = fmtCtx->streams[audioStreamIndex]->time_base;

    if(avcodec_open2(audioCodecCtx,audioCodec,NULL)<0){
        printf("Cannot open audio codec.\n");
        return 0;
    }

    //设置转码参数
    uint64_t out_channel_layout = audioCodecCtx->channel_layout;
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    out_sample_rate = audioCodecCtx->sample_rate;
    out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    //printf("out rate : %d , out_channel is: %d\n",out_sample_rate,out_channels);

    audio_out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE*2);

    swr_ctx = swr_alloc_set_opts(NULL,
                                 out_channel_layout,
                                 out_sample_fmt,
                                 out_sample_rate,
                                 audioCodecCtx->channel_layout,
                                 audioCodecCtx->sample_fmt,
                                 audioCodecCtx->sample_rate,
                                 0,NULL);
    if(swr_init(swr_ctx)!=0){
        printf("Cannot init swr\n");
        return 0;
    }
    ///////////////////////////音频部分结束//////////////////////////////////

    return true;
}

void FFmpegAudio::run()
{
    if(!open_input_file()){
        qDebug()<<"Please open audio file first.";
        return;
    }

    qDebug()<<"Open audio file "<<_url<<"done.";

    double sleep_time=0;

    while(av_read_frame(fmtCtx,pkt)>=0){
        if(pkt->stream_index==audioStreamIndex){
            if(avcodec_send_packet(audioCodecCtx,pkt)>=0){
                while(avcodec_receive_frame(audioCodecCtx,audioFrame)>=0){
                    if(av_sample_fmt_is_planar(audioCodecCtx->sample_fmt)){
                        int len = swr_convert(swr_ctx,
                                              &audio_out_buffer,
                                              MAX_AUDIO_FRAME_SIZE*2,
                                              (const uint8_t**)audioFrame->data,
                                              audioFrame->nb_samples);
                        if(len<=0){
                            continue;
                        }
                        //qDebug("convert length is: %d.\n",len);

                        int out_size = av_samples_get_buffer_size(0,
                                                                  out_channels,
                                                                  len,
                                                                  out_sample_fmt,
                                                                  1);

                        sleep_time=(out_sample_rate*16*2/8)/out_size;

                        if(audioOutput->bytesFree()<out_size){
                            msleep(sleep_time);
                            streamOut->write((char*)audio_out_buffer,out_size);
                        }else {
                            streamOut->write((char*)audio_out_buffer,out_size);
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }
    }

    qDebug()<<"All audio play done";
}

FFmpegVideo::FFmpegVideo()
{
    img_ctx = sws_alloc_context();
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

bool FFmpegVideo::open_input_file()
{
    if(_url.isEmpty()) return 0;

    if(avformat_open_input(&fmtCtx,_url.toLocal8Bit().data(),NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return 0;
    }

    if(avformat_find_stream_info(fmtCtx,NULL)<0){
        printf("Cannot find any stream in file.\n");
        return 0;
    }

    int streamCnt=fmtCtx->nb_streams;
    for(int i=0;i<streamCnt;i++){
        if(fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStreamIndex = i;
            continue;
        }
    }

    if(videoStreamIndex==-1){
        printf("Cannot find any stream in file.\n");
        return 0;
    }

    //////////////////////视频部分开始/////////////////////////////
    AVCodecParameters *videoCodecPara = fmtCtx->streams[videoStreamIndex]->codecpar;

    if(!(videoCodec = avcodec_find_decoder(videoCodecPara->codec_id))){
        printf("Cannot find valid decode codec.\n");
        return 0;
    }

    if(!(videoCodecCtx = avcodec_alloc_context3(videoCodec))){
        printf("Cannot find valid decode codec context.\n");
        return 0;
    }

    if(avcodec_parameters_to_context(videoCodecCtx,videoCodecPara)<0){
        printf("Cannot initialize parameters.\n");
        return 0;
    }
    if(avcodec_open2(videoCodecCtx,videoCodec,NULL)<0){
        printf("Cannot open codec.\n");
        return 0;
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
        return 0;
    }
    ///////////////////////////视频部分结束//////////////////////////////////

    return true;
}

void FFmpegVideo::run()
{
    if(!open_input_file()){
        qDebug()<<"Please open video file first.";
        return;
    }

    qDebug()<<"Open video file "<<_url<<"done.";

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

    fa = new FFmpegAudio;
    connect(fa,&FFmpegAudio::finished,fa,&FFmpegAudio::deleteLater);
}

FFmpegWidget::~FFmpegWidget()
{
    if(ffmpeg->isRunning()){
        stop();
    }

    if(fa->isRunning()){
        stop();
    }
}

void FFmpegWidget::setUrl(QString url)
{
    ffmpeg->setUrl(url);
    fa->setUrl(url);
}

void FFmpegWidget::play()
{
    stop();
    ffmpeg->start();
    fa->start();
}

void FFmpegWidget::stop()
{
    if(ffmpeg->isRunning()){
        ffmpeg->requestInterruption();
        ffmpeg->quit();
        ffmpeg->wait(100);
    }

    if(fa->isRunning()){
        fa->requestInterruption();
        fa->quit();
        fa->wait(100);
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
