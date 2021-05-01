#include "ffmpegwidget.h"

FFmpegVideo::FFmpegVideo()
{
}

FFmpegVideo::~FFmpegVideo()
{}

void FFmpegVideo::init_variables()
{
    fmtCtx = avformat_alloc_context();
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    yuvFrame = av_frame_alloc();
    rgbFrame = av_frame_alloc();

    avformat_network_init();

    runFlag=true;
}

void FFmpegVideo::free_variables()
{
    if(!pkt) av_packet_free(&pkt);
    if(!yuvFrame) av_frame_free(&yuvFrame);
    if(!rgbFrame) av_frame_free(&rgbFrame);
    if(!videoCodecCtx) avcodec_free_context(&videoCodecCtx);
    if(!videoCodecCtx) avcodec_close(videoCodecCtx);
    if(!fmtCtx) avformat_close_input(&fmtCtx);
    if(!filterGraph) avfilter_graph_free(&filterGraph);

    runFlag=false;
}

void FFmpegVideo::setUrl(QString url)
{
    _url =url;
}

int FFmpegVideo::open_input_file()
{
    init_variables();//初始化变量

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

    av_dump_format(fmtCtx,videoStreamIndex,_url.toLocal8Bit().data(),0);

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
    videoWidth = videoCodecCtx->width;
    videoHeight = videoCodecCtx->height;

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

    initFilter();//初始化滤镜

    return true;
}

void FFmpegVideo::initFilter()
{
    /*
     * 开始初始化滤波器
    * Filter的具体定义，只要是libavfilter中已注册的filter，
    * 就可以直接通过查询filter名字的方法获得其具体定义，所谓
    * 定义即filter的名称、功能描述、输入输出pad、相关回调函数等
    */
    AVFilter *bufSrc = (AVFilter*)avfilter_get_by_name("buffer");/* 输入buffer filter */
    AVFilter *bufSink = (AVFilter*)avfilter_get_by_name("buffersink");/* 输出buffer filter */
    //AVFilterInOut对应buffer和buffersink这两个首尾端的filter的输入输出
    AVFilterInOut *outFilter = avfilter_inout_alloc();
    AVFilterInOut *inFilter = avfilter_inout_alloc();

    enum AVPixelFormat pixel_fmts[]={AV_PIX_FMT_YUV420P,AV_PIX_FMT_NONE};

    filterGraph = avfilter_graph_alloc();
    if(!outFilter||!inFilter||!filterGraph){
        ret = AVERROR(ENOMEM);
    }

    QString qargs=QString("video_size=%1x%2:pix_fmt=0:time_base=1/20")
            .arg(videoWidth)
            .arg(videoHeight);
    char* args=qargs.toLocal8Bit().data();

    //根据指定的Filter，这里就是buffer，构造对应的初始化参数args，二者结合即可创建Filter的示例，并放入filter_graph中
    int ret = avfilter_graph_create_filter(&bufSrcCtx,
                                           bufSrc,
                                           "in",
                                           args,NULL,
                                           filterGraph);
    if(ret<0){
        printf("Cannot create buffer source.\n");
    }

    /* buffer video sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&bufSinkCtx,
                                       bufSink,
                                       "out",
                                       NULL,
                                       NULL,
                                       filterGraph);
    if(ret<0){
        printf("Cannot creat buffer sink.\n");
    }

    ret = av_opt_set_int_list(bufSinkCtx, "pix_fmts", pixel_fmts,
                              AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
    }

    /* Endpoints for the filter graph. */
    outFilter->name = av_strdup("in");
    outFilter->filter_ctx = bufSrcCtx;
    outFilter->pad_idx =0;
    outFilter->next = NULL;

    inFilter->name = av_strdup("out");
    inFilter->filter_ctx = bufSinkCtx;
    inFilter->pad_idx = 0;
    inFilter->next = NULL;

    //filter_descr是一个filter命令，例如"overlay=iw:ih"，该函数可以解析这个命令，
    //然后自动完成FilterGraph中各个Filter之间的联接
    ret = avfilter_graph_parse_ptr(filterGraph,
                                   //filterDesc,
                                   filterDescr.toStdString().data(),
                                   &inFilter,
                                   &outFilter,
                                   NULL);
    if(ret<0){
        printf("Cannot parse filter desc.\n");
    }

    //检查当前所构造的FilterGraph的完整性与可用性
    if(avfilter_graph_config(filterGraph,NULL)<0){
        printf("Check config failed.\n");
    }
}

void FFmpegVideo::setCL(int c, int l)
{
    c=c<1?1:c;
    c=c>9?9:c;
    l=l<1?1:l;
    l=l>9?9:l;

    float cf = 1+((float)(c-5))/10.0;
    float lf = 0+((float)(l-5))/10.0;

    filterDescr = QString("eq=contrast=%1:brightness=%2").arg(cf).arg(lf);
}

void FFmpegVideo::run()
{
    if(!open_input_file()){
        qDebug()<<"Please open file first.";
        return;
    }

    AVFrame *filterFrame = av_frame_alloc();
    while(av_read_frame(fmtCtx,pkt)>=0){
        if(!runFlag){
            break;
        }
        initFilter();
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

                    /* push the decoded frame into the filtergraph */
                    if (av_buffersrc_add_frame_flags(bufSrcCtx, yuvFrame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
                        break;
                    }

                    //把滤波后的视频帧从filter graph取出来
                    if(av_buffersink_get_frame(bufSinkCtx,filterFrame)<0){
                        printf("Cannot get frame from bufSinkCtx.\n");
                        continue;
                    }

                    sws_scale(img_ctx,
                              filterFrame->data,filterFrame->linesize,
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

void FFmpegWidget::play(QString url)
{
    stop();
    ffmpeg->setUrl(url);
    ffmpeg->start();
}

void FFmpegWidget::stop()
{
    if(ffmpeg->isRunning()){
        ffmpeg->requestInterruption();
        ffmpeg->quit();
        ffmpeg->wait(100);
    }
    ffmpeg->free_variables();
    img.fill(Qt::black);
}

void FFmpegWidget::setFilterDescr(int c, int b)
{
    ffmpeg->setCL(c,b);
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
