//    QString fileName = "test/1.avi";
    QString fileName = "E:/testFile/3.mp4";
//    QString fileName = "E:/testFile2/1.mp3";

    QString outFileName = "D:/1.pcm";

    AVFormatContext *pAVFormatContext = 0;          // ffmpeg的全局上下文，所有ffmpeg操作都需要
    AVCodecContext *pAVCodecContext = 0;            // ffmpeg编码上下文
    AVCodec *pAVCodec = 0;                          // ffmpeg编码器
    AVPacket *pAVPacket = 0;                        // ffmpag单帧数据包
    AVFrame *pAVFrame = 0;                          // ffmpeg单帧缓存
    SwrContext *pSwrContext = 0;                    // ffmpeg音频转码
    QFile file(outFileName);                        // Qt文件操作

    int ret = 0;                                    // 函数执行结果
    int audioIndex = -1;                            // 音频流所在的序号
    int numBytes = 0;
    uint8_t * outData[2] = {0};
    int dstNbSamples = 0;                           // 解码目标的采样率

    int outChannel = 0;                             // 重采样后输出的通道
    AVSampleFormat outFormat = AV_SAMPLE_FMT_NONE;  // 重采样后输出的格式
    int outSampleRate = 0;                          // 重采样后输出的采样率

    pAVFormatContext = avformat_alloc_context();    // 分配
    pAVPacket = av_packet_alloc();                  // 分配
    pAVFrame = av_frame_alloc();                    // 分配

    if(!pAVFormatContext || !pAVPacket || !pAVFrame)
    {
        LOG << "Failed to alloc";
        goto END;
    }
    // 步骤一：注册所有容器和编解码器（也可以只注册一类，如注册容器、注册编码器等）
    av_register_all();

    // 步骤二：打开文件(ffmpeg成功则返回0)
    LOG << "文件:" << fileName << "，是否存在：" << QFile::exists(fileName);
//    ret = avformat_open_input(&pAVFormatContext, fileName.toUtf8().data(), pAVInputFormat, 0);
    ret = avformat_open_input(&pAVFormatContext, fileName.toUtf8().data(), 0, 0);
    if(ret)
    {
        LOG << "Failed";
        goto END;
    }

    // 步骤三：探测流媒体信息
    ret = avformat_find_stream_info(pAVFormatContext, 0);
    if(ret < 0)
    {
        LOG << "Failed to avformat_find_stream_info(pAVCodecContext, 0)";
        goto END;
    }
    LOG << "视频文件包含流信息的数量:" << pAVFormatContext->nb_streams;

    // 步骤四：提取流信息,提取视频信息
    for(int index = 0; index < pAVFormatContext->nb_streams; index++)
    {
        pAVCodecContext = pAVFormatContext->streams[index]->codec;
        switch (pAVCodecContext->codec_type)
        {
        case AVMEDIA_TYPE_UNKNOWN:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_UNKNOWN";
            break;
        case AVMEDIA_TYPE_VIDEO:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_VIDEO";
            break;
        case AVMEDIA_TYPE_AUDIO:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_AUDIO";
            audioIndex = index;
            break;
        case AVMEDIA_TYPE_DATA:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_DATA";
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_SUBTITLE";
            break;
        case AVMEDIA_TYPE_ATTACHMENT:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_ATTACHMENT";
            break;
        case AVMEDIA_TYPE_NB:
            LOG << "流序号:" << index << "类型为:" << "AVMEDIA_TYPE_NB";
            break;
        default:
            break;
        }
        // 已经找打视频品流
        if(audioIndex != -1)
        {
            break;
        }
    }

    if(audioIndex == -1 || !pAVCodecContext)
    {
        LOG << "Failed to find video stream";
        goto END;
    }
    // 步骤五：对找到的音频流寻解码器
    pAVCodec = avcodec_find_decoder(pAVCodecContext->codec_id);
    if(!pAVCodec)
    {
        LOG << "Fialed to avcodec_find_decoder(pAVCodecContext->codec_id):"
            << pAVCodecContext->codec_id;
        goto END;
    }
#if 0
    pAVCodecContext = avcodec_alloc_context3(pAVCodec);
    // 填充CodecContext信息
    if (avcodec_parameters_to_context(pAVCodecContext,
                                      pAVFormatContext->streams[audioIndex]->codecpar) < 0)
    {
        printf("Failed to copy codec parameters to decoder context!\n");
        goto END;
    }
#endif

    // 步骤六：打开解码器
    ret = avcodec_open2(pAVCodecContext, pAVCodec, NULL);
    if(ret)
    {
        LOG << "Failed to avcodec_open2(pAVCodecContext, pAVCodec, pAVDictionary)";
        goto END;
    }

    // 打印
    LOG << "解码器名称：" <<pAVCodec->name << endl
        << "通道数：" << pAVCodecContext->channels << endl
        << "通道布局：" << av_get_default_channel_layout(pAVCodecContext->channels) << endl
        << "采样率：" << pAVCodecContext->sample_rate << endl
        << "采样格式：" << pAVCodecContext->sample_fmt;
#if 1
    outChannel = 2;
    outSampleRate = 44100;
    outFormat = AV_SAMPLE_FMT_S16P;
#endif
#if 0
    outChannel = 2;
    outSampleRate = 48000;
    outFormat = AV_SAMPLE_FMT_FLTP;
#endif
    LOG << "to" << endl
        << "通道数：" << outChannel << endl
        << "通道布局：" << av_get_default_channel_layout(outChannel) << endl
        << "采样率：" << outSampleRate << endl
        << "采样格式：" << outFormat;
    // 步骤七：获取音频转码器并设置采样参数初始化
    // 入坑二：通道布局与通道数据的枚举值是不同的，需要转换
    pSwrContext = swr_alloc_set_opts(0,                                 // 输入为空，则会分配
                                     av_get_default_channel_layout(outChannel),
                                     outFormat,                         // 输出的采样频率
                                     outSampleRate,                     // 输出的格式
                                     av_get_default_channel_layout(pAVCodecContext->channels),
                                     pAVCodecContext->sample_fmt,       // 输入的格式
                                     pAVCodecContext->sample_rate,      // 输入的采样率
                                     0,
                                     0);
    ret = swr_init(pSwrContext);
    if(ret < 0)
    {
        LOG << "Failed to swr_init(pSwrContext);";
        goto END;
    }

    file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    outData[0] = (uint8_t *)av_malloc(1152 * 8);
    outData[1] = (uint8_t *)av_malloc(1152 * 8);

    // 步骤七：读取一帧数据的数据包
    while(av_read_frame(pAVFormatContext, pAVPacket) >= 0)
    {
        if(pAVPacket->stream_index == audioIndex)
        {
            // 步骤八：将封装包发往解码器
            ret = avcodec_send_packet(pAVCodecContext, pAVPacket);
            if(ret)
            {
                LOG << "Failed to avcodec_send_packet(pAVCodecContext, pAVPacket) ,ret =" << ret;
                break;
            }
            // 步骤九：从解码器循环拿取数据帧
            while(!avcodec_receive_frame(pAVCodecContext, pAVFrame))
            {
                // nb_samples并不是每个包都相同，遇见过第一个包为47，第二个包开始为1152的
//                LOG << pAVFrame->nb_samples;
                // 步骤十：获取每个采样点的字节大小
                numBytes = av_get_bytes_per_sample(outFormat);
                // 步骤十一：修改采样率参数后，需要重新获取采样点的样本个数
                dstNbSamples = av_rescale_rnd(pAVFrame->nb_samples,
                                              outSampleRate,
                                              pAVCodecContext->sample_rate,
                                              AV_ROUND_ZERO);
                // 步骤十二：重采样
                swr_convert(pSwrContext,
                            outData,
                            dstNbSamples,
                            (const uint8_t **)pAVFrame->data,
                            pAVFrame->nb_samples);
                // 第一次显示
                static bool show = true;
                if(show)
                {
                    LOG << numBytes << pAVFrame->nb_samples << "to" << dstNbSamples;
                    show = false;
                }
                // 步骤十四：使用LRLRLRLRLRL（采样点为单位，采样点有几个字节，交替存储到文件，可使用pcm播放器播放）
                for (int index = 0; index < dstNbSamples; index++)
                {
                    for (int channel = 0; channel < pAVCodecContext->channels; channel++)  // 交错的方式写入, 大部分float的格式输出
                    {
                        //  用于原始文件jinxin跟对比
//                        file.write((char *)pAVFrame->data[channel] + numBytes * index, numBytes);
                        file.write((char *)outData[channel] + numBytes * index, numBytes);
                    }
                }
                av_free_packet(pAVPacket);
            }
        }
    }
    file.close();
END:
    LOG << "释放回收资源";
    if(outData[0] && outData[1])
    {
        av_free(outData[0]);
        av_free(outData[1]);
        outData[0] = 0;
        outData[1] = 0;
        LOG << "av_free(outData[0])";
        LOG << "av_free(outData[1])";
    }
    if(pSwrContext)
    {
        swr_free(&pSwrContext);
        pSwrContext = 0;
    }
    if(pAVFrame)
    {
        av_frame_free(&pAVFrame);
        pAVFrame = 0;
        LOG << "av_frame_free(pAVFrame)";
    }
    if(pAVPacket)
    {
        av_free_packet(pAVPacket);
        pAVPacket = 0;
        LOG << "av_free_packet(pAVPacket)";
    }
    if(pAVCodecContext)
    {
        avcodec_close(pAVCodecContext);
        pAVCodecContext = 0;
        LOG << "avcodec_close(pAVCodecContext);";
    }
    if(pAVFormatContext)
    {
        avformat_close_input(&pAVFormatContext);
        avformat_free_context(pAVFormatContext);
        pAVFormatContext = 0;
        LOG << "avformat_free_context(pAVFormatContext)";
    }
}