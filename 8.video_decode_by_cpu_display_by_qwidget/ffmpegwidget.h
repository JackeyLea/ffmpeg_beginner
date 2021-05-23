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

    void setUrl(QString url);

    int open_input_file();

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

    struct SwsContext *img_ctx=NULL;

    unsigned char *out_buffer=nullptr;

    int videoStreamIndex =-1;
    int numBytes = -1;

    QString _url;
};

class FFmpegWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FFmpegWidget(QWidget *parent = nullptr);
    ~FFmpegWidget();

    void setUrl(QString url);

    void play();
    void stop();

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void receiveQImage(const QImage &rImg);

private:
    FFmpegVideo *ffmpeg;

    QImage img;
};

#endif // FFMPEGWIDGET_H
