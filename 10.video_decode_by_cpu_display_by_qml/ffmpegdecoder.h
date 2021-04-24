#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QThread>
#include <QDebug>
#include <QString>
#include <QVector>
#include <QImage>
#include <QContiguousCache>

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

const static int bufferSize = 1024*768;

struct YUVData{
    YUVData(){
        Y.reserve(bufferSize);
        U.reserve(bufferSize);
        V.reserve(bufferSize);
    }
    QByteArray Y;
    QByteArray U;
    QByteArray V;
    int yLineSize;
    int uLineSize;
    int vLineSize;
    int height;
};

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
    YUVData getFrame(){
        if(frameBuffer.isEmpty()){
            return YUVData{};
        }
        return frameBuffer.takeFirst();
    }

protected:
    void run();

signals:
    void sigFirst(uchar* p,int w,int h);
    void newFrame();
    void videoInfoReady(int w,int h);

private:
    AVFormatContext *fmtCtx       =NULL;
    AVCodec         *videoCodec   =NULL;
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

    YUVData m_yuvData;

    QContiguousCache<YUVData> frameBuffer;
};

#endif // FFMPEGDECODER_H
