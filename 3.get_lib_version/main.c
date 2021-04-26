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
    unsigned codecVer = avcodec_version();
    int ver_major,ver_minor,ver_micro;
    ver_major = (codecVer>>16)&0xff;
    ver_minor = (codecVer>>8)&0xff;
    ver_micro = (codecVer)&0xff;
    printf("FFmpeg version is: %s .\navcodec version is: %d=%d.%d.%d.\n",FFMPEG_VERSION,codecVer,ver_major,ver_minor,ver_micro);

    return 0;
}
