# ffmpeg_beginner

<a href="https://feater.top/ffmpeg/ffmpeg-learning-indexes" target="_blank">FFmpeg4/5/6入门系列教程</a>代码

## 编译说明

- 使用<a href="https://github.com/FFmpeg/FFmpeg" target="_blank">FFmpeg</a>tags中4/5/6版本最新版本号源码编译测试
- 最近忙着驻场，没有太多时间维护代码，所以使用条件编译的方式适配各个版本，不会有各个版本的独立分支
- 尽量保证编译结果没有错误、没有警告、没有deprecated方法调用

## 源码说明

### 3.get_lib_version

获取库版本信息并解析输出可读信息

### 4.get_stream_info

输出视频的基本信息（时长、码率、编码方式等等）

### 5.video_decode_flow

视频解码的基本流程

### 6.video_decode_frame_save

解码视频并保存其中的50帧画面为ppm格式图片

### 7.video_decode_mp42yuv

视频解码的基本流程并输出视频信息，将解码后的视频数据保存为YUV格式文件

分别解码为YUV420P/YUV420SP

### 8.video_decode_by_cpu_display_by_qwidget 

使用CPU解码视频，然后使用Qt的QWidget显示画面

### 9.video_decode_by_cpu_display_by_qopengl

使用CPU解码视频，然后使用Qt的QOpenGL显示画面

两种方法仅供参考

### 10.video_decode_by_cpu_display_by_qml

使用CPU解码视频，然后使用QML显示画面

### 11.video_decode_by_cuda_display_by_qt

使用CUDA解码视频并使用Qt的QWidget/QOpenGL/QML显示视频

### 12.video_encode_yuv2h264

将yuv源视频文件编码为h264格式的文件

### 13.video_encode_h2642mp4

将h264编码为mp4格式文件

### 14.video_encode_camera2h264

将摄像头捕获的视频直接编码为H264格式

### 15.audio_decode_mp32pcm

将mp3文件解码为pcm文件

### 16.audio_decode_swr_mp32pcm

将mp3音频重采样解码为pcm

### 17.audio_player_decode_by_ffmpeg_play_by_qt

使用FFmpeg解码音频，使用Qt播放音频

### 18.audio_player_decode_from_mem_play_by_qt

解码内存中的mp3数据并使用Qt播放

### 19.audio_encode_pcm2mp3

将pcm格式文件编码为mp3格式

### 20.audio_video_sync

Qt简单视频播放器，带音视频同步

### 21.video_decode_add_filter_display_by_qwidget

使用CPU解码视频，并添加滤镜，然后使用QWidget显示画面

### 22.video_demuxer_mp42h264mp3

将mp4分解为h264和mp3

### 23.video_demuxer_mp42yuvpcm

将mp4分解为h264和mp3，并在此基础上将h264解码为yuv，将mp3解码为pcm

### 24.video_muxer_mp3h2642mp4

将h264和mp3合并为mp4

### RTSParser

收RTSP流，并解析流中的H264数据

### 待添加

本系列的目的就是将雷霄华同志的教程进行新版本适配，其在CSDN发布的所有文章涉及的代码都会进行适配