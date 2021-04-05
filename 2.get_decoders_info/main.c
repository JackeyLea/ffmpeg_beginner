#include <stdio.h>
#include <stdlib.h>

#include <libavcodec/avcodec.h>

int main()
{
    char *info = (char *)malloc(40000);
    memset(info, 0, 40000);

    void *i = 0;
    AVCodec *p;

    while((p=(AVCodec*)av_codec_iterate(&i))){
        if(p->decode != NULL){
            strcat(info, "[Decode]");
        }else{
            strcat(info, "[Encode]");
        }

        switch(p->type){
        case AVMEDIA_TYPE_VIDEO:
            strcat(info, "[Video]");
            break;
        case AVMEDIA_TYPE_AUDIO:
            strcat(info, "[Audio]");
            break;
        default:
            strcat(info, "[Other]");
            break;
        }
        sprintf(info,"%s %10s\n",info,p->name);
    }
    fprintf(stderr, "%s %s\n",__FUNCTION__,info);
    free(info);
    return 0;
}
