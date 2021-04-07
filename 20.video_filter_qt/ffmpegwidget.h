#ifndef FFMPEGWIDGET_H
#define FFMPEGWIDGET_H

#include <QImage>
#include <QWidget>
#include <QPaintEvent>
#include <QThread>
#include <QPainter>
#include <QDebug>

#include <string>

extern "C"{
#include <libavcodec/avcodec.h>

#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/mem.h>

#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <libswscale/swscale.h>

#include <libavformat/avformat.h>
}

using namespace std;

class FFmpegVideo : public QThread
{
    Q_OBJECT
public:
    explicit FFmpegVideo();
    ~FFmpegVideo();

    void init_variables();
    void free_variables();

    void setUrl(QString url);
    int open_input_file();

    void initFilter();

    void setCL(int c,int l);

protected:
    void run();

signals:
    void sendQImage(QImage);

private:
    AVFormatContext *fmtCtx       =NULL;
    AVCodec         *videoCodec   =NULL;
    AVCodecContext  *videoCodecCtx=NULL;
    AVPacket        *pkt          = NULL;
    AVFrame         *yuvFrame     = NULL;
    AVFrame         *rgbFrame     = NULL;

    AVFilterContext *bufSinkCtx;
    AVFilterContext *bufSrcCtx;
    AVFilterGraph *filterGraph;

    QString filterDescr="eq=contrast=1:brightness=0";
    QString _url;

    struct SwsContext *img_ctx=NULL;

    unsigned char *out_buffer;

    int videoStreamIndex =-1;
    int numBytes = -1;

    bool runFlag=true;
    int ret=-1;
};

class FFmpegWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FFmpegWidget(QWidget *parent = nullptr);
    ~FFmpegWidget();

    void play(QString url);
    void stop();

    void setFilterDescr(int c,int b);

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void receiveQImage(const QImage &rImg);

private:
    FFmpegVideo *ffmpeg;

    QImage img;
};

#endif // FFMPEGWIDGET_H
