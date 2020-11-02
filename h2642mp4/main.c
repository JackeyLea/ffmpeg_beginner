#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"

//刷新编码器
int flush_encoder(AVFormatContext *fmtCtx, AVCodecContext *codecCtx,int vStreamIndex) {
    int      ret;
    AVPacket enc_pkt;
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    if (!(codecCtx->codec->capabilities & AV_CODEC_CAP_DELAY))
        return 0;

    printf("Flushing stream #%u encoder\n",vStreamIndex);
    if(avcodec_send_frame(codecCtx,0)>=0){
        while(avcodec_receive_packet(codecCtx,&enc_pkt)>=0){
            printf("success encoder 1 frame.\n");

            // parpare packet for muxing
            enc_pkt.stream_index = vStreamIndex;
            av_packet_rescale_ts(&enc_pkt,codecCtx->time_base,
                                 fmtCtx->streams[ vStreamIndex ]->time_base);
            ret = av_interleaved_write_frame(fmtCtx, &enc_pkt);
            if(ret<0){
                break;
            }
        }
    }

    return ret;
}
// 在将H264转封装为MP4
int main(){
    AVOutputFormat *outFmt =NULL;
    AVFormatContext *inVFmtCtx=NULL,*inAFmtCtx=NULL,*outFmtCtx=NULL;

    int ret=-1;
    int inVStreamIndex=-1,outVStreamIndex=-1;
    const char *inVFileName = "result.h264";
    const char *outFileName = "result.mp4";

    if(avformat_open_input(&inVFmtCtx,inVFileName,NULL,NULL)<0){
        printf("Cannot open input file.\n");
        return -1;
    }

    if(avformat_find_stream_info(inVFmtCtx,NULL)<0){
        printf("Cannot find stream info in input file.\n");
        return -1;
    }

    for(int i=0;i<inVFmtCtx->nb_streams;i++){
        if(inVFmtCtx->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_VIDEO){
            inVStreamIndex=i;
            break;
        }
    }

    AVCodecParameters *codecPara = inVFmtCtx->streams[inVStreamIndex]->codecpar;

    printf("===============Input information========>\n");
    av_dump_format(inVFmtCtx, 0, inVFileName, 0);
    printf("===============Input information========<\n");

    if(avformat_alloc_output_context2(&outFmtCtx,NULL,NULL,outFileName)<0){
        printf("Cannot alloc output file context.\n");
        return -1;
    }
    outFmt = outFmtCtx->oformat;

    if(avio_open(&outFmtCtx->pb,outFileName,AVIO_FLAG_READ_WRITE)<0){
        printf("output file open failed.\n");
        return -1;
    }

    AVStream *outVStream = avformat_new_stream(outFmtCtx,NULL);
    if(!outVStream){
        printf("Failed allocating output stream.\n");
        return -1;
    }
    outVStreamIndex=outVStream->index;
    AVCodec *outCodec = avcodec_find_encoder(codecPara->codec_id);
    if(outCodec==NULL){
        printf("Cannot find any encoder.\n");
        return -1;
    }
    AVCodecContext *outCodecCtx=avcodec_alloc_context3(outCodec);
    printf("%d",outCodecCtx->codec_id);
    AVCodecParameters *outCodecPara = outFmtCtx->streams[outVStream->index]->codecpar;
    if(avcodec_parameters_copy(outCodecPara,codecPara)<0){
        printf("Cannot copy codec para.\n");
        return -1;
    }
    if(avcodec_parameters_to_context(outCodecCtx,outCodecPara)<0){
        printf("Cannot alloc codec ctx from para.\n");
        return -1;
    }

    if(avcodec_open2(outCodecCtx,outCodec,NULL)<0){
        printf("Cannot open output codec.\n");
        return -1;
    }

    printf("============Output Information=============>\n");
    av_dump_format(outFmtCtx,0,outFileName,1);
    printf("============Output Information=============<\n");



    return 0;
}
