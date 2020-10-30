#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/ffversion.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libpostproc/postprocess.h"

int main()
{
    AVFormatContext *fmt_ctx = avformat_alloc_context();//创建对象并初始化
    int ret;
    char* fileName="beat.mp4";//文件地址

    //打开文件
    if ((ret = avformat_open_input(&fmt_ctx, fileName, NULL, NULL))<0)
        return ret;//Cannot open video file

    //查找流信息（音频流和视频流）
    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    av_dump_format(fmt_ctx,0,fileName,0);//输出视频信息

    avformat_close_input(&fmt_ctx);//关闭文件
    
    return 0;
}
