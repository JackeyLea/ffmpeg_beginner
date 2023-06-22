#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QThread>
#include <QDebug>
#include <QString>
#include <QVector>
#include <QImage>

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


class FFmpegDecoder : public QThread
{
    Q_OBJECT
public:
    FFmpegDecoder();
    ~FFmpegDecoder();

    void setUrl(QString const url);

    int width();
    int height();

    ///
    /// \brief getFrame 从解码结果缓存队列中取第一帧显示
    /// \return 第一帧数据指针
    ///
    uchar* getFrame(){
        return out_buffer;
    }

protected:
    void run();

signals:
    void sigFirst(uchar* p,int w,int h);
    void newFrame();

private:
    AVFormatContext *fmtCtx       =NULL;
    const AVCodec         *videoCodec   =NULL;
    AVCodecContext  *videoCodecCtx=NULL;
    AVPacket        *pkt          = NULL;
    AVFrame         *yuvFrame     = NULL;
    AVFrame         *rgbFrame     = NULL;

    struct SwsContext *img_ctx=NULL;

    uchar *out_buffer= nullptr;

    int videoStreamIndex =-1;
    int numBytes = -1;

    QString _url;

    bool isFirst = true;

    int w,h;
};

#endif // FFMPEGDECODER_H
