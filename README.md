# ffmpeg_beginner

ffmpeg入门系列教程代码

使用之前先修改ffmpeg.pri文件中ffmpeg库文件的位置

## 源码

### 1.get_lib_version

输出FFmpeg库的版本号并解析

### 2.get_decoders_info

输出FFmpeg库的编解码器信息

### 3.get_stream_info

输出视频的基本信息（时长、码率、编码方式等等）

### 4.video_decode_mp42yuv

视频解码的基本流程并输出视频信息，将解码后的视频数据保存为YUV格式文件

### 5.video_decode_frame_save

解码视频并保存其中的50帧画面为ppm格式图片

### 6.video_encode_yuv2h264

将yuv源视频文件编码为h264格式的文件

### 7.video_encode_h2642mp4

将h264编码为mp4格式文件

### 8.video_encode_camera2h264

将摄像头捕获的视频直接编码为H264格式

### 9.video_player_decode_by_cpu_display_by_qwidget

使用FFmpeg软解视频并使用Qt显示画面

### 10.video_player_decode_by_cuda_display_by_qwidget

使用FFmpeg的CUDA库加速解码视频并使用QWidget显示画面

### 11.video_player_decode_by_cuda_display_by_qopengl

使用FFmpeg的CUDA库加速解码视频并使用QOpenGLWidget显示画面

### ffmpeg_audio_decode_mp32pcm

将mp3文件解码为pcm文件

### ffmpeg_audio_decode_mp32pcm_swr

将mp3音频重采样解码为pcm

### ffmpeg_audio_encode_pcm2mp3

将pcm格式文件编码为mp3格式