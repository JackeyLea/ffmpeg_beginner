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
int main(int argc, char *argv[]) {
    AVOutputFormat *outFmt=NULL;
    //Input AVFormatContext and Output AVFormatContext
    AVFormatContext *inVFmtCtx=NULL,*inAFmtCtx=NULL,*outFmtCtx=NULL;
    AVPacket pkt;
    int ret=-1, i;
    int inVIndex=0,outVIndex=0;
    int frameIndex = 0;
    int64_t cur_pts_v = 0, cur_pts_a = 0;
    const char *inVFileName = "result.h264";
    const char *outFileName = "222.mp4";//Output file URL
    //Input
    if ((ret = avformat_open_input(&inVFmtCtx, inVFileName, 0, 0)) < 0) {
        printf("Could not open input file.");
        return -1;
    }

    if ((ret = avformat_find_stream_info(inVFmtCtx, 0)) < 0) {
        printf("Failed to retrieve input stream information");
        return -1;
    }

    printf("===============Input information=========\n");
    av_dump_format(inVFmtCtx, 0, inVFileName, 0);

    //Output
    avformat_alloc_output_context2(&outFmtCtx, NULL, NULL, outFileName);
    if (!outFmtCtx) {
        printf("Could not create output context\n");
        return -1;
    }
    outFmt = outFmtCtx->oformat;


    AVCodecParameters *codecPara = inVFmtCtx->streams[inVIndex]->codecpar;
    AVStream *in_stream = inVFmtCtx->streams[inVIndex];
    AVStream *out_stream = avformat_new_stream(outFmtCtx, in_stream->codec->codec);
    //Copy the settings of AVCodecContext
    if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
        printf("Failed to copy context from input to output stream codec context\n");
        goto end;
    }
    out_stream->codec->codec_tag = 0;
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    //break;

    printf("==========Output Information==========\n");
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    printf("======================================\n");

    //Write file header
    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        printf("Error occurred when opening output file\n");
        goto end;
    }

    while (1) {
        AVFormatContext *ifmt_ctx;
        int stream_index = 0;
        AVStream *in_stream, *out_stream;
        //Get an AVPacket
        //if(av_compare_ts(cur_pts_v,ifmt_ctx_v->streams[videoindex_v]->time_base,cur_pts_a,ifmt_ctx_a->streams[audioindex_a]->time_base) <= 0)
        {
            ifmt_ctx = ifmt_ctx_v;
            stream_index = videoindex_out;
            if (av_read_frame(ifmt_ctx, &pkt) >= 0) {
                do {
                    in_stream = ifmt_ctx->streams[pkt.stream_index];
                    out_stream = ofmt_ctx->streams[stream_index];
                    printf("stream_index==%d,pkt.stream_index==%d,videoindex_v=%d\n", stream_index, pkt.stream_index, videoindex_v);
                    if (pkt.stream_index == videoindex_v) {
                        //FIX：No PTS (Example: Raw H.264)
                        //Simple Write PTS
                        if (pkt.pts == AV_NOPTS_VALUE) {
                            printf("frame_index==%d\n", frame_index);
                            //Write PTS
                            AVRational time_base1 = in_stream->time_base;
                            //Duration between 2 frames (us)
                            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
                            //Parameters
                            pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
                            pkt.dts = pkt.pts;
                            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
                            frame_index++;
                        }
                        cur_pts_v = pkt.pts;
                        break;
                    }
                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
            }
            else {
                break;
            }
        }

        //Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index = stream_index;
        printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt.size, pkt.pts);
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
            printf("Error muxing packet\n");
            break;
        }
        av_free_packet(&pkt);
    }
    //Write file trailer
    av_write_trailer(ofmt_ctx);

    avformat_close_input(&ifmt_ctx_v);
    //avformat_close_input(&ifmt_ctx_a);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }

    return 0;
}
