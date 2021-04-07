#ifndef FFMPEGVIDEO_H
#define FFMPEGVIDEO_H

#include <QImage>
#include <QWidget>
#include <QPaintEvent>
#include <QThread>
#include <QPainter>
#include <QDebug>
#include <QDateTime>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QMutex>

#include <string>
#include <iostream>
#include <ostream>

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/parseutils.h>
#include <libavutil/error.h>
#include <libavutil/buffer.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_qsv.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

using namespace std;

class FFmpegVideo : public QThread
{
    Q_OBJECT
public:
    FFmpegVideo(QObject *parent=nullptr);
    ~FFmpegVideo();

    void setUrl(QString url);
    int open_input_file();
    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                            const enum AVPixelFormat *pix_fmts);
    static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type);
protected:
    void run();

signals:
    void sigStarted(uchar *,int,int);
    void sigNewFrame();

private:
    QString _filePath;
    uchar *out_buffer;
    int ret =0;
    bool isFirst = true;
};

#endif // FFMPEGVIDEO_H
