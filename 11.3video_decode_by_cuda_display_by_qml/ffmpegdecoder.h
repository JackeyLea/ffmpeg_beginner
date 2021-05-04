#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QThread>
#include <QDebug>

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

class FFmpegDecoder : public QThread
{
    Q_OBJECT
public:
    FFmpegDecoder();
    ~FFmpegDecoder();

    void setUrl(QString url);

    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,
                                            const enum AVPixelFormat *pix_fmts);
    static int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type);

    int width(){return videoWidth;}
    int height(){return videoHeight;}

    uchar* getFrame(){
        return out_buffer;
    }

protected:
    void run();

signals:
    void sigNewFrame();

private:
    QString _filePath;
    uchar* out_buffer;

    int ret=0;

    int videoWidth=0,videoHeight=0;
};

#endif // FFMPEGDECODER_H
